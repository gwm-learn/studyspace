FROM ubuntu:16.04

# path
ENV PATH_PACKAGE /root/package
ENV PATH_PACKAGE_SOURCE_DIR ./package

ENV PATH_PACKAGE_PYTHON $PATH_PACKAGE/Python-2.7.6
ENV PATH_PACKAGE_NINJA $PATH_PACKAGE/ninja-1.3.4
ENV PATH_PACKAGE_PERL $PATH_PACKAGE/perl-5.18.4

# packege
ENV PACKAGE_PYTHON $PATH_PACKAGE_PYTHON.tgz
ENV PACKAGE_NINJA $PATH_PACKAGE_NINJA.zip
ENV PACKAGE_GN $PATH_PACKAGE/gn
ENV PACKAGE_PERL $PATH_PACKAGE_PERL.tar.gz

# copy packege to image
COPY $PATH_PACKAGE_SOURCE_DIR $PATH_PACKAGE

# install dependent packages
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
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
    subversion \
    texi2html \
    texinfo \
    unzip \
    tar \
    flex \
    libtinfo5 \
    libxml2-dev  \
    libncurses-dev
# python
RUN cd $PATH_PACKAGE \
    && tar -xvzf $PACKAGE_PYTHON \
    && cd $PATH_PACKAGE_PYTHON \
    && ./configure --with-ssl \
    && make \
    && make install \
    && ln -s /usr/local/bin/python /usr/bin/python
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
    && make test \
    ; make install \
    && cpan install Spreadsheet/ParseExcel.pm
# npm
RUN apt-get install -y --no-install-recommends  curl \
    && curl -sL https://deb.nodesource.com/setup_lts.x | bash - \
    && apt-get install -y --no-install-recommends nodejs npm \
    && npm install -g n \
    && n 10.16.3
# root
RUN echo "export set FORCE_UNSAFE_CONFIGURE=1" >> /etc/profile \
    && echo "source /etc/profile" >> /root/.bashrc





