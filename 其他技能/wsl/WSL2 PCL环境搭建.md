# WSL2 PCL环境搭建
## 环境搭建
```bash
sudo apt-get autoremove libboost1.71-dev libboost-all-dev
sudo apt-get install  mpi-default-dev libicu-dev python-dev libbz2-dev libboost-dev libboost-dev libboost-all-dev gcc g++ pkg-config clang-format-12  libeigen3-dev libflann-dev  libusb-dev libusb-1.0 libqhull-dev libglew-dev cmake-gui libflann-dev libvtk7-dev flex bison
```

```bash
wget https://www.coin-or.org/download/source/metslib/metslib-0.5.3.tgz
tar xzvf metslib-0.5.3.tgz
cd metslib-0.5.3
./configure
make -j`nproc`
sudo make install
```

```bash
https://sourceforge.net/projects/libpng/files/libpng16/1.6.37/
tar xzf libpng-1.6.37.tar.gz
cd libpng-1.6.37
./configure --prefix=/usr/local/libpng
make
make install
```

```bash
http://www.tcpdump.org/#latest-release
sudo apt-get install 
tar -zxvf libpcap-1.10.1.tar.gz 
cd libpcap-1.10.1/
sudo ./configure
sudo make
sudo make install
```

```bash
https://vtk.org/download/
tar xzf VTK-8.2.0.tar.gz
cd VTK-8.2.0
mkdir build
cd build
cmake ..
make -j4
sudo make install
```

## 下载源码

```bash
https://github.com/PointCloudLibrary/pcl/releases
wget -c https://github.com/PointCloudLibrary/pcl/releases/download/pcl-1.11.1/source.tar.gz
```

## 解压

```bash
tar -zxvf source.tar.gz
mv pcl pcl-pcl-1.11.1
cd pcl-pcl-1.11.1
```


## 编译

```bash
cmake-gui -DCMAKE_BUILD_TYPE=Release ..
选中BUILD_visualization
make -j4
sudo make -j4 install
```
