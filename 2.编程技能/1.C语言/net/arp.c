#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>
#include <netinet/ip.h>        // struct iphdr
#include <netinet/ip_icmp.h>   // struct icmphdr, ICMP_ECHO, ICMP_ECHOREPLY

// 获取接口MAC地址
int get_interface_mac_ex(const char *interface, unsigned char *mac) {
    struct ifreq ifr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        printf("socket fail");
        return -1;
    }
    
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        printf("ioctl(SIOCGIFHWADDR) fail");
        close(sock);
        return -1;
    }
    
    memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    close(sock);
    return 0;
}

// 获取接口IP地址
in_addr_t get_interface_ip_ex(const char *interface) {
    struct ifreq ifr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    in_addr_t ip = 0;
    
    if (sock < 0) {
        printf("socket fail");
        return INADDR_ANY;
    }
    
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
        ip = sin->sin_addr.s_addr;
    } else {
        printf("ioctl(SIOCGIFADDR) fail");
    }
    
    close(sock);
    return ip;
}

int arp_probe(const char *target_mac_str, const char *interface, const char *target_ip_str) {
    int online = 0;
    int sock = -1;

    // 1. 准备网络接口信息
    unsigned char if_mac[ETH_ALEN];
    if (get_interface_mac_ex(interface, if_mac) < 0) {
        printf("Failed to get MAC for interface %s", interface);
        return 0;
    }
    
    in_addr_t if_ip = get_interface_ip_ex(interface);
    if (if_ip == INADDR_ANY) {
        printf("Failed to get IP for interface %s", interface);
        return 0;
    }
    
    // 2. 转换目标MAC和IP
    unsigned char target_mac[ETH_ALEN];
    if (sscanf(target_mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
              &target_mac[0], &target_mac[1], &target_mac[2],
              &target_mac[3], &target_mac[4], &target_mac[5]) != 6) {
        printf("Invalid target MAC format: %s", target_mac_str);
        return 0;
    }
    
    in_addr_t target_ip = inet_addr(target_ip_str);
    if (target_ip == INADDR_NONE) {
        printf("Invalid target IP: %s", target_ip_str);
        return 0;
    }
    
    // 3. 创建原始套接字
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sock < 0) {
        printf("socket(AF_PACKET, SOCK_RAW, ETH_P_ARP) fail");
        return 0;
    }
    
    // 4. 绑定到指定接口
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        printf("ioctl(SIOCGIFINDEX) fail");
        close(sock);
        return 0;
    }
    
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ARP);
    
    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        printf("bind fail");
        close(sock);
        return 0;
    }
    
    // 5. 构造ARP请求包
    unsigned char packet[60]; // 最小ARP包大小
    memset(packet, 0, sizeof(packet));
    
    // 以太网帧头
    struct ethhdr *eth = (struct ethhdr *)packet;
    memset(eth->h_dest, 0xFF, ETH_ALEN); // 广播地址: FF:FF:FF:FF:FF:FF
    memcpy(eth->h_source, if_mac, ETH_ALEN); // 源MAC = 接口MAC
    eth->h_proto = htons(ETH_P_ARP); // ARP协议
    
    // ARP头部
    struct arphdr *arp = (struct arphdr *)(packet + sizeof(struct ethhdr));
    arp->ar_hrd = htons(ARPHRD_ETHER); // 以太网硬件类型
    arp->ar_pro = htons(ETH_P_IP);     // IPv4协议类型
    arp->ar_hln = ETH_ALEN;            // MAC地址长度
    arp->ar_pln = 4;                   // IP地址长度
    arp->ar_op = htons(ARPOP_REQUEST); // ARP请求
    
    // ARP数据部分
    unsigned char *arp_data = packet + sizeof(struct ethhdr) + sizeof(struct arphdr);
    
    // 发送方MAC (接口MAC)
    memcpy(arp_data, if_mac, ETH_ALEN);
    arp_data += ETH_ALEN;
    
    // 发送方IP (接口IP)
    memcpy(arp_data, &if_ip, 4);
    arp_data += 4;
    
    // 目标MAC (初始为全0)
    memset(arp_data, 0, ETH_ALEN);
    arp_data += ETH_ALEN;
    
    // 目标IP (要探测的IP)
    memcpy(arp_data, &target_ip, 4);
    
    // 6. 发送ARP请求
    if (sendto(sock, packet, sizeof(packet), 0, 
              (struct sockaddr *)&sll, sizeof(sll)) <= 0) {
        printf("sendto fail");
        close(sock);
        return 0;
    }
    
    // 7. 设置接收超时
    struct timeval tv = {1, 0}; // 1秒超时
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // 8. 接收并解析响应
    unsigned char recv_buf[1024];
    while (1) {
        int n = recv(sock, recv_buf, sizeof(recv_buf), 0);
        if (n <= 0) {
            // 超时或错误
            printf("recv fail");
            break;
        }
        
        // 检查以太网类型
        struct ethhdr *recv_eth = (struct ethhdr *)recv_buf;
        if (ntohs(recv_eth->h_proto) != ETH_P_ARP) {
            continue; // 不是ARP包
        }
        
        // 检查ARP操作码
        struct arphdr *recv_arp = (struct arphdr *)(recv_buf + sizeof(struct ethhdr));
        if (ntohs(recv_arp->ar_op) != ARPOP_REPLY) {
            continue; // 不是ARP响应
        }
        
        // 提取ARP数据部分
        unsigned char *recv_arp_data = recv_buf + sizeof(struct ethhdr) + sizeof(struct arphdr);
        
        // 检查发送方IP是否匹配目标IP
        in_addr_t recv_sender_ip = *((in_addr_t *)(recv_arp_data + ETH_ALEN));
        if (recv_sender_ip != target_ip) {
            continue; // IP不匹配
        }
        
        // 检查发送方MAC是否匹配目标MAC
        if (memcmp(recv_arp_data, target_mac, ETH_ALEN) == 0) {
            online = 1; // MAC匹配，设备在线
            printf("recv success");
            break;
        }
    }
    
    close(sock);
    return online;
}