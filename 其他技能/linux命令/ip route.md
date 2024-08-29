1. 添加静态路由
ip ro add 8.8.8.8 dev ccmni1 metric 99
ip ro add 8.8.8.8 via 192.168.225.1 metric 100