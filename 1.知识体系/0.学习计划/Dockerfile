FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive


# apt换成阿里云源
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list


# OpenWrt完整编译依赖，新增 python3-distutils
RUN apt-get update && apt-get install -y \
ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
bzip2 ccache clang cmake cpio curl device-tree-compiler flex gawk gettext git gperf \
haveged help2man intltool libelf-dev libgmp3-dev libltdl-dev libmpc-dev libmpfr-dev \
libncurses-dev libreadline-dev libssl-dev libtool ninja-build p7zip-full patch pkgconf \
python3 python3-distutils python3-setuptools python3-ply python3-docutils qemu-utils re2c rsync scons squashfs-tools \
subversion swig texinfo uglifyjs upx-ucl unzip vim wget xmlto xz-utils zlib1g-dev \
net-tools iputils-ping dnsutils openssh-client procps htop strace gdb jq tree file


# 修复sh指向bash（openwrt部分脚本依赖）
RUN ln -sf /bin/bash /bin/sh


# npm nodejs（LuCI前端编译需要）
RUN apt install -y nodejs npm && npm config set registry https://registry.npmmirror.com


# pip国内镜像
RUN mkdir -p /root/.pip && \
echo "[global]" > /root/.pip/pip.conf && \
echo "index-url = https://mirrors.aliyun.com/pypi/simple/" >> /root/.pip/pip.conf && \
echo "trusted-host = mirrors.aliyun.com" >> /root/.pip/pip.conf


# ==========核心：和宿主机同步UID/GID 1000==========
RUN groupadd -g 1000 gwm && \
    useradd -u 1000 -g 1000 -m -s /bin/bash gwm && \
    echo 'gwm ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers


# gwm用户pip镜像
USER gwm
RUN mkdir -p /home/gwm/.pip && \
echo "[global]" > /home/gwm/.pip/pip.conf && \
echo "index-url = https://mirrors.aliyun.com/pypi/simple/" >> /home/gwm/.pip/pip.conf && \
echo "trusted-host = mirrors.aliyun.com" >> /home/gwm/.pip/pip.conf


USER root


ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8


WORKDIR /w
# 默认使用普通用户gwm运行
USER gwm
