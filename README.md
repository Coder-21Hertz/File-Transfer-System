# File-Transfer-System
网络文件安全传输系统，其中利用TCP协议进行文件传输，SSL技术进行加密，同时结合线程池实现多台客户机同时向服务机上传或下载文件的功能。

> 本系统参考了[dayL_W](https://blog.csdn.net/u013181595)的文章完成：[网络安全传输系统](https://so.csdn.net/so/search/s.do?q=%E7%BD%91%E7%BB%9C%E5%AE%89%E5%85%A8%E4%BC%A0%E8%BE%93%E7%B3%BB%E7%BB%9F&t=blog&u=u013181595)。

### 具体功能

- 基于C/S模型（Client/Server），实现文件安全的上传和下载功能；
- 利用TCP协议进行文件的传输；
- 通过OpenSSL技术对TCP包的明文数据进行加密再传输；
- 采用线程池技术，实现多个客户机同时访问服务器的功能；
- 采用MySQL数据库实现客户端的账号管理功能。

### 使用方法

- 本系统需要先安装的环境有：

  书籍Linux/Unix系统编程手册的API接口函数[https://www.cnblogs.com/pluse/p/6296992.html](https://www.cnblogs.com/pluse/p/6296992.html)

  MYSQL以及OpenSSL库

- 第一次使用还要在服务端创建一个数据库

  数据库学的不深，学完在回来改。具体过程：

  ```sql
  -- 创建数据库 project_ssl_login
  CREATE DATABASE project_ssl_login character set utf8;
  -- 选择数据库
  use project_ssl_login;
  -- 创建一个数据表 login
  CREATE TABLE login(
  username VARCHAR(20),
  password VARCHAR(20),
  unique(username)
  );
  ```

- 编译文件

  ```bash
  # 服务端
  gcc server.c common.c pthread_pool.c login.c -o server -ltlpi -lssl -lcrypto -ldl -lpthread -I/usr/include/mysql/ -lmysqlclient -Wall -g
  # 客户端
  gcc client.c common.c pthread_pool.c login.c -o client -ltlpi -lssl -lcrypto -ldl -lpthread -I/usr/include/mysql/ -lmysqlclient -Wall -g
  ```

- 服务端生成公钥和私钥

  ```bash
  # 产生私钥
  openssl genrsa -out privkey.pem 2048
  # 产生公钥
  openssl req -new -x509 -key privkey.pem -out cacert.pem -days 1095
  ```

- 可以愉快的运行使用啦

  ```bash
  # 服务端
  ./server cacert.pem privkey.pem   
  # 客户端
  ./client ip地址
  ```

  