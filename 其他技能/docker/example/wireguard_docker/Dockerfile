FROM ubuntu:16.04

# path
ENV PATH_PACKAGE /root/package
ENV PATH_PACKAGE_SOURCE_DIR ./package

ENV PATH_PACKAGE_PYTHON $PATH_PACKAGE/Python-2.7.6

# packege
ENV PACKAGE_PYTHON $PATH_PACKAGE_PYTHON.tgz

# copy packege to image
COPY $PATH_PACKAGE_SOURCE_DIR $PATH_PACKAGE

COPY ./wireguard/systemctl /bin/systemctl

# install dependent packages
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    wireguard \
    net-tools \
    iproute2 \
    iptables \
    openresolv \
    gcc \
    g++

# python
RUN cd $PATH_PACKAGE \
    && tar -xvzf $PACKAGE_PYTHON \
    && cd $PATH_PACKAGE_PYTHON \
    && ./configure --with-ssl \
    && make \
    && make install \
    && ln -s /usr/local/bin/python /usr/bin/python

RUN chmod a+x /bin/systemctl

# root
RUN echo "export set FORCE_UNSAFE_CONFIGURE=1" >> /etc/profile \
    && echo "source /etc/profile" >> /root/.bashrc





