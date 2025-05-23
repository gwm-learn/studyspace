FROM docker.m.daocloud.io/library/ubuntu:14.04

# path
ENV PATH_CONF /root/conf
ENV PATH_MIBS /root/mibs
ENV PATH_PACKAGE /root/package

ENV PATH_CONF_SOURCE_DIR ./conf
ENV PATH_MIBS_SOURCE_DIR ./mibs
ENV PATH_PACKAGE_SOURCE_DIR ./package

ENV SNMPD_CONF $PATH_CONF/snmpd.conf
ENV SNMPTRAPD_CONF $PATH_CONF/snmptrapd.conf
ENV SOURCES_LIST $PATH_CONF/sources.list
ENV SOURCE_MIBS $PATH_MIBS/BL-VENDOR-MIB.txt

COPY $PATH_CONF_SOURCE_DIR $PATH_CONF
COPY $PATH_MIBS_SOURCE_DIR $PATH_MIBS
COPY $PATH_PACKAGE_SOURCE_DIR $PATH_PACKAGE

RUN cp $SOURCES_LIST /etc/apt/sources.list

# install dependent packages
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    vim \
    net-tools \
    python2.7 \
    make \
    gcc \
    gcc-multilib

# install snmp
RUN cd $PATH_PACKAGE \
    && tar xzvf $PATH_PACKAGE/net-snmp-5.9.4.tar.gz
RUN cd $PATH_PACKAGE/net-snmp-5.9.4 \
    && ./configure --prefix=/usr/local/net-snmp \
    --with-default-snmp-version='2' \
    --with-sys-contact='xxx@xxx.com' \
    --with-sys-location='location' \
    --with-logfile='/var/log/snmpd.log' \
    --with-persistent-directory='/usr/local/net-snmp' \
    --without-opaque-special-types \
    --without-rpm--without-perl-modules \
    --disable-manuals \
    --disable-ucd-snmp-compatibility \
    --disable-embedded-perl \
    --disable-snmptrapd-subagent \
    --disable-scripts \
    --enable-mfd-rewrites \
    --enable-shared=no \
    --with-endianness=little
RUN cd $PATH_PACKAGE/net-snmp-5.9.4 \
    && make \
    && make install
RUN cp $SOURCE_MIBS /usr/local/net-snmp/share/snmp/mibs/BL-VENDOR-MIB.txt

# root
RUN echo "export set FORCE_UNSAFE_CONFIGURE=1" >> /etc/profile \
    && echo "export PATH=/usr/local/net-snmp/bin:/usr/local/net-snmp/sbin:$PATH" >> /etc/profile \
    && echo "source /etc/profile" >> /root/.bashrc





