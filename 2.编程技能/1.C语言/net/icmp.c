#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>
#include <netinet/ip.h>        // struct iphdr
#include <netinet/ip_icmp.h>   // struct icmphdr, ICMP_ECHO, ICMP_ECHOREPLY

int icmp_ping(const char *ip, const char *interface) {
    int online = 0;

    // 创建原始套接字（需要root权限）
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        printf("icmp ping socket fail");
        return online;
    }
    
    // 设置目标地址
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &dest_addr.sin_addr) <= 0) {
        printf("icmp pping inet_pton fail");
        close(sock);
        return online;
    }
    
    // 构造ICMP Echo请求
    struct icmphdr icmp_req;
    memset(&icmp_req, 0, sizeof(icmp_req));
    icmp_req.type = ICMP_ECHO;
    icmp_req.code = 0;
    icmp_req.un.echo.id = getpid();
    icmp_req.un.echo.sequence = 0;
    icmp_req.checksum = in_cksum((unsigned short*)&icmp_req, sizeof(icmp_req));
    
    // 发送请求
    if (sendto(sock, &icmp_req, sizeof(icmp_req), 0, 
              (struct sockaddr*)&dest_addr, sizeof(dest_addr)) <= 0) {
        printf("icmp ping sendto fail");
        close(sock);
        return online;
    }
    
    // 设置接收超时
    struct timeval tv = {1, 0}; // 1秒超时
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 绑定到特定接口
    struct ifreq if_idx;
    strncpy(if_idx.ifr_name, interface, IFNAMSIZ);
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &if_idx, sizeof(if_idx)) < 0) {
        perror("setsockopt(SO_BINDTODEVICE)");
        close(sock);
        return 0;
    }
    
    // 接收响应
    char recv_buf[1024];
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);

    while (1) {
        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                        (struct sockaddr*)&src_addr, &addrlen);
        if (n <= 0) {
            printf("icmp ping recv_from fail");
            break; 
        }// 超时或错误
        
        // 检查来源IP是否匹配
        if (src_addr.sin_addr.s_addr != dest_addr.sin_addr.s_addr) {
            continue;
        }
        
        // 解析IP头
        struct iphdr *ip_hdr = (struct iphdr*)recv_buf;
        int iphdr_len = ip_hdr->ihl * 4;
        
        // 检查包长度
        if (n < iphdr_len + sizeof(struct icmphdr)) {
            continue; // 无效包
        }
        
        // 解析ICMP头
        struct icmphdr *icmp_rep = (struct icmphdr*)(recv_buf + iphdr_len);
        
        // 验证响应
        if (icmp_rep->type == ICMP_ECHOREPLY) {
            online = 1;
            printf("icmp ping success");
            break;
        }
    }
    
    close(sock);
    return online;
}