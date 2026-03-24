FROM docker.m.daocloud.io/library/ubuntu:22.04

# path
ENV PATH_PACKAGE /root/package
ENV PATH_PACKAGE_SOURCE_DIR ./package

ENV PATH_PACKAGE_PYTHON $PATH_PACKAGE/Python-3.10.12
ENV PATH_PACKAGE_NINJA $PATH_PACKAGE/ninja-1.3.4
ENV PATH_PACKAGE_PERL $PATH_PACKAGE/perl-5.40.0

# packege
ENV PACKAGE_PYTHON $PATH_PACKAGE_PYTHON.tgz
ENV PACKAGE_NINJA $PATH_PACKAGE_NINJA.zip
ENV PACKAGE_GN $PATH_PACKAGE/gn
ENV PACKAGE_PERL $PATH_PACKAGE_PERL.tar.gz

# copy packege to image
COPY $PATH_PACKAGE_SOURCE_DIR $PATH_PACKAGE

# change sources.list
RUN mv /etc/apt/sources.list /etc/apt/sources.list.bak \
    && cp $PATH_PACKAGE/sources.list /etc/apt/sources.list

# install dependent packages
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    sudo \
    subversion \
    bison \
    gcc \
    g++ \
    make \
    re2c \
    build-essential \
    libc6-dev-i386 \
    wget \
    coreutils \
    diffstat \
    gawk \
    git \
    git-core \
    help2man \
    libtool \
    quilt \
    sed \
    texi2html \
    texinfo \
    unzip \
    tar \
    flex \
    libtinfo5 \
    libxml2-dev  \
    libncurses-dev \
    zlib1g-dev \
    rsync \
    curl \
    ca-certificates \
    netbase \
    openssl \
    libssl-dev \
    libffi-dev \
    musl-dev \
    cmake \
    autoconf \
    automake \
    pkg-config \
    file \
    tree \
    patch \
    cpio \
    net-tools \
    iputils-ping \
    dnsutils \
    telnet \
    openssh-client \
    procps \
    htop \
    lsof \
    strace \
    gdb \
    vim \
    nano \
    less \
    jq \
    bc \
    time

# python
RUN cd $PATH_PACKAGE \
    && tar -xvzf $PACKAGE_PYTHON \
    && cd $PATH_PACKAGE_PYTHON \
    && ./configure \
        --prefix=/usr/local \
        --with-openssl=/usr \
        --with-ssl \
        --with-openssl-rpath=auto \
        --enable-optimizations \
    && make \
    && make install \
    && ln -s /usr/local/bin/python3 /usr/bin/python
    
# ninja
RUN cd $PATH_PACKAGE \
    && unzip $PACKAGE_NINJA \
    && cd $PATH_PACKAGE_NINJA \
    && ./bootstrap.py \
    && cp ninja /usr/bin
# gn
RUN cd $PATH_PACKAGE \
    && chmod 777 $PACKAGE_GN \
    && cp gn /usr/bin/
# perl
RUN cd $PATH_PACKAGE \
    && tar -xzf $PACKAGE_PERL \
    && cd $PATH_PACKAGE_PERL \
    && ./Configure -des -Dprefix=$HOME/localperl \
    && make \
    && make install \
    && cpan install Spreadsheet/ParseExcel.pm
# npm
RUN curl -sL https://deb.nodesource.com/setup_lts.x | bash - \
    && apt-get install -y --no-install-recommends nodejs npm \
    && npm install -g n

# pip
RUN python3 -m ensurepip --upgrade \
    && python3 -m pip install --upgrade "pip<21.0" \
        -i https://mirrors.aliyun.com/pypi/simple/ \
        --trusted-host mirrors.aliyun.com \
    && python3 -m pip install "cryptography==3.3.2" \
        -i https://mirrors.aliyun.com/pypi/simple/ \
        --trusted-host mirrors.aliyun.com

# 安装 Python 2 和 pip
RUN apt-get install -y --no-install-recommends python2 \
    && wget https://bootstrap.pypa.io/pip/2.7/get-pip.py \
    && python2.7 get-pip.py \
    && pip2 install "enum34==1.1.10" \
        -i https://mirrors.aliyun.com/pypi/simple/

# root
RUN echo "export set FORCE_UNSAFE_CONFIGURE=1" >> /etc/profile \
    && echo "source /etc/profile" >> /root/.bashrc

# 同步容器与宿主机环境
# 创建开发用户（UID/GID与宿主机gwm用户匹配）
RUN groupadd -g 1000 gwm && \
    useradd -u 1000 -g 1000 -m -s /bin/bash gwm && \
    echo 'gwm ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
# 设置工作目录
WORKDIR /w/
# 默认以gwm用户运行
USER gwm
# 设置 locale 环境变量
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8