

# 基于muduo和protobuf的微服务RPC框架

## [框架结构]


![框架示意图](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC.JPG)

## [功能描述]：
本项目主要是基于muduo+protobuf+zookeeper的微服务RPC框架。服务方可以通过mprpc框架注册rpc节点发布服务。客户端可以通过zookeeper服务配置中心发现服务，并进行远程方法的调用。

## [开发环境]：
1. protobuf 3.11.0
2. gcc version 7.5.0
3. Zookeeper version: 3.4.10


## [测试DEMO]：
位于bin目录下的provider和consumer。

    ./provider -i test.conf 
    ./consumer -i test.conf

# MPRPC项目笔记



![](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC_detail/image-20220718165055903.png)



## 一、服务提供者callee

1. 首先服务提供者使用protobuf注册一个RPC方法。也就是写一个`.proto`结尾的文件。

其中`FriendServiceRpc`是远程服务RPC类的名称，里面定义了调用方法的名称`GetFriendList`和其参数类型`GetFriendListRequest`，还指定了返回的类型`GetFriendListRespone`。

```protobuf
service FriendServiceRpc {
    rpc GetFriendList(GetFriendListRequest) returns(GetFriendListRespone);
}
```

然后使用指令进行编译，因此会产生一个头文件`friend.pb.h`可供使用：

```c++
protoc friend.proto --cpp_out ./
```

2. 然后服务提供者需要继承 `friend.pb.h`文件中的`GetFriendList`类，并重写`GetFriendList`这个函数：

```c++
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"
#include <vector>
#include <string> 

class FriendService : public fixbug::FriendServiceRpc 
{
    // 获取好友列表 本地实现
    std::vector<std::string> GetFriendList(u_int32_t id, std::string name, std::string pwd) {
        
        std::cout << "GetFriendList Local" << std::endl;
        std::cout << "id:" << id << " name:" << name << " pwd:" << pwd << std::endl;

        std::vector<std::string> friendVec;
        friendVec.push_back("Simon1");
        friendVec.push_back("Simon2");
        friendVec.push_back("Simon3");

        return friendVec;
    }

    void GetFriendList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendListRequest* request,
                       ::fixbug::GetFriendListRespone* response,
                       ::google::protobuf::Closure* done)
    {
        u_int32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        // Test
        std::cout << "id:" << id << std::endl;
        std::cout << "name:" << name << std::endl;
        std::cout << "pwd:" << pwd << std::endl; 
        // 执行本地getFriendList方法
        std::vector<std::string> friendVec = GetFriendList(id, name, pwd);
        // 填充rpc响应
        for (int i = 0; i < friendVec.size(); ++i) {
            response->add_friendlist(friendVec[i].c_str());
        }
        response->set_success(true);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        
        // 调用done进行序列化和网络发送 由框架完成
        done->Run();
    }
};

int main(int argc, char **argv) {
    // 测试日志
    // LOG_INFO("first log message");
    // LOG_ERR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);
    
    // 初始化框架配置
    MprpcApplication::Init(argc, argv);

    // 注册一个rpc服务对象 将FriendService发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动服务
    provider.run();
}
```

在需要重写的函数中，提供了四个参数：

- controller：
- request : 服务调用者在调用方法时传入的参数，在这里把参数拿出来传入本地的函数中进行实际的工作。
- response：执行完结果的返回值。注意这里四个参数全是指针，只需要把对应的内容填好即可。
- done：执行done的run方法。

重写好之后，服务提供者需要使用我们的框架进行初始化，然后发布一个RPC结点：

1. 实例化一个`rpcprovider`对象，调用这个对象的`NotifyService`函数，把继承后重写的类new一个出来，作为

`NotifyService`函数的参数。

2. 启动`rpcprovider`对象的run函数，就可以发布成功了。

```c++
int main(int argc, char **argv) {
    // 测试日志
    // LOG_INFO("first log message");
    // LOG_ERR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);
    
    // 初始化框架配置
    MprpcApplication::Init(argc, argv);

    // 注册一个rpc服务对象 将FriendService发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动服务
    provider.run();
}
```



> 1. 框架的初始化做了什么？
> 2. RpcProvider类怎么实现注册RPC方法的？
> 3. RpcProvider类的run方法发生了什么？



## 二、服务调用方caller

在前面服务提供方利用`friend.proto`文件生成的头文件中，其实还定义了一个`FriendServiceRpc_stub`类，这个类是给服务的调用方提供的。

1. 作为服务使用方，首先需要调用框架的初始化。
2. 然后实例化一个`FriendServiceRpc_Stub`类型的对象，这个stub对象需要new一个`MprpcChannel`对象传进去。
3. 填写双方约定好的方法调用时的参数，并初始化好一个约定好的函数返回类型的传出参数的指针，用来接收函数的返回值。
4. 初始化一个MprpcController对象，用来判断是否调用成功。
5. 调用FriendServiceRpc_Stub对象的`GetFrinedList`函数，把`contoller、request、respone`的地址传进去。
6. 从controll对象判断是否远程调用成功，如果成功就可以从`respone`中取出返回值了。

```c++
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "mprpccontroller.h"


int main(int argc, char** argv) {
    MprpcApplication::Init(argc, argv);
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());

    // 填写参数
    fixbug::GetFriendListRequest request;
    request.set_id(100);
    request.set_name("IS-Simon");
    request.set_pwd("666666");
    // 输出参数 得到的响应
    fixbug::GetFriendListRespone respone;
    // 控制对象 如果发生了错误 会记录在controller中
    MprpcController controller;
    // rpc远程调用 其实是调用channel callmethod
    stub.GetFriendList(&controller, &request, &respone, nullptr);

    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    } else {
        if (respone.result().errcode() == 0) {
            std::cout << "rpc GetFriendList success!" << std::endl;
            // 打印好友列表
            for (int i = 0; i < respone.friendlist_size(); ++i) {
                std::cout << "idx:" << i << " " << respone.friendlist(i) << std::endl;
            }
        } else {
            std::cout << "rpc GetFriendList fail! error code:" << respone.result().errcode()
                << "error msg:" << respone.result().errmsg() << std::endl;
        }
    }   
    exit(0);
}
```



> 1. 框架的初始化做了什么？
> 2. 为什么stub类的对象需要传入一个MprpcChannel的对象？
> 3. 什么时候调用不成功/成功？



## 三、框架类

![image-20220717185501947](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC_detail/image-20220717185501947.png)



### 1. Mprpcapplication类 和 MprpcConfig类

框架类主要有`Mprpcapplication` 和`MprpcConfig`、`rpcprovider`三个类。

对于`Mprpcapplication`类的`init`函数：接受一个配置文件的名字，然后调用配置文件类的对象把配置文件的信息写到配置文件类的哈希表中。



![image-20220717193751961](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC_detail/image-20220717193751961.png)



### 2. rpcprovider类

`rpcprovider`这个类是最重要的。它完成了注册服务、连接客户端并接收请求、处理完请求关闭连接、处理客户端的可读信息（解析）、序列化响应并进行返回。

1. `NotifyService`这个函数是用使用框架的`callee`来使用的。它负责接受一个服务，并把它注册到自己维护的`m_serviceMap`表中。这个服务是用一个虚基类指针Service* 来接受的。使用框架的callee只需要把自己重写的类new一个传进去即可。在传进去之后，由于传进去的服务也是是继承自Service虚基类的，因此可以拿出指向这个服务的指针`ServiceDescriptor*`，服务的名字，还可以拿出服务中对应方法的数量。知道了数量之后，可以使用一个循环把所有的方法取出来`MethodDescriptor*`。然后其实每个方法都是有名字的，因此可以把服务-方法保存为一个结构体，那就是上面的 `ServiceInfo`结构体。

2. `run`函数负责将前面使用框架的callee注册的服务注册到ZooKeeper中，同时开启一个服务器来接受客户端的连接请求。当这个服务器有新的数据可读时，就会调用`OnMessage`函数。而如果有客户端断开连接时，就会调用`OnConnection`函数。

3. `OnConnection`函数负责断开连接的客户端`shutdown`掉。

4. `OnMesssage`函数负责解析客户端的请求。在这个请求中，数据已经被放入一个`muduo::net::Buffer*`类型的buffer中，这个类型可以提供一个`retrieveAllAsString`函数，把收到的数据变成string类型的字符串。在这个字符串中，为了避免TCP的粘包问题，收到的消息可以分为 ：

   - 前4个字节是消息头的大小，然后可以得到表示消息头的string。
   - 把表示消息头的string反序列化，可以得到服务名、方法名、参数的size
   - 按参数的size截取出响应的string大小，通过反序列化可以得到参数的protobuf（Message）类型。

   完成了这些之后，就可以从provider类中维护的映射表中获取服务的`ServiceDescriptor*`，再用`Service*`来接受。然后就可以调用`Service*` 的`CallMethod`方法了。这方法是由protobuf进行调用的：

   ```c++
   // 给下面的method调用
       google::protobuf::Closure* done = 
                           google::protobuf::NewCallback<RpcProvider, 
                           const muduo::net::TcpConnectionPtr&, 
                           google::protobuf::Message*>
                           (this, &RpcProvider::SendRpcRespone, conn, respone);
   
       // protobuf根据远端的rpc请求，调用当前节点上RPC发布的方法 
       service->CallMethod(method, nullptr, request, respone, done);
       /*
           其实就是调用了callee里面的函数，因为有method 所以protobuf可以知道调用的是哪个方法
           在调用之后，就会跑到前面callee重写的类里面
           在重写的类执行完本地业务后 还需要执行done函数
           其实就是执行了SendRpcRespone
       */ 
   ```

   callMethod方法最后其实是调用了使用框架的callee的重写的类中，调用的函数是：

   ```c++
   void GetFriendList(::google::protobuf::RpcController* controller,
                          const ::fixbug::GetFriendListRequest* request,
                          ::fixbug::GetFriendListRespone* response,
                          ::google::protobuf::Closure* done)
       {
           u_int32_t id = request->id();
           std::string name = request->name();
           std::string pwd = request->pwd();
   
           // Test
           std::cout << "id:" << id << std::endl;
           std::cout << "name:" << name << std::endl;
           std::cout << "pwd:" << pwd << std::endl; 
           // 执行本地getFriendList方法
           std::vector<std::string> friendVec = GetFriendList(id, name, pwd);
           // 填充rpc响应
           for (int i = 0; i < friendVec.size(); ++i) {
               response->add_friendlist(friendVec[i].c_str());
           }
           response->set_success(true);
           response->mutable_result()->set_errcode(0);
           response->mutable_result()->set_errmsg("");
           
           // 调用done进行序列化和网络发送 由框架完成
           done->Run();
       }
   ```

   在这里，callee还要调用done的run方法，这部分主要是由`SendRpcRespone`函数完成的。

   5. 由于`SendRpcRespone`函数会拿到上面callee执行完本地函数的指针，所以这个函数的作用是进行`respone`信息的反序列化，并使用`send`函数把反序列化之后的数据发送给方法的调用者caller。

### 3. mprpcchannel类

![image-20220717215223324](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC_detail/image-20220717215223324.png)



在前面caller调用RPC方法的过程中，其实是`MprpcChannel`这个类执行了真正的服务。想想看caller只做了什么？

在caller里面，它只是初始化出来一个服务，然后调用这个服务的方法，再把对应的函数参数（request）和传出参数（respone）放进了要调用的服务方法中。

那么谁把消息序列化呢？使用服务的caller怎么知道服务端的IP地址在哪儿呢？谁把消息发送出去呢？这一切都是`Mprpcchannel`这个类做的事情。它继承自`RpcChannel`这个虚基类，通过重写`callMethod`方法来完成以上的事情。

在这个类中，主要做的事情有：

- 格式化发送的消息。发送的消息需要 “消息头长度+消息头（服务名+方法名+参数长度）+参数”，需要把这些数据整理好。
- 在整理好之后，还需要根据服务的名字+方法的名字向Zookeeper获取服务提供者的IP、端口号。
- 然后进行SOCKET通信，完成消息的发送。
- 接收RPC服务提供者的消息，进行反序列化，填好传出参数`respone`

如果在上面的这一系列步骤中，有步骤出错，那么就需要`controller`这个类来发挥作用了。

### 4.Mprpccontroller类

这个类继承自`RpcController`这个类。它主要由`mprpcchannel`类进行调用的。主要重写了一些失败时的函数。caller如果在一些步骤上调用失败，就会被controller记录，然后被caller知道。



## 四、Zookeeper类



![image-20220718142450686](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC_detail/image-20220718142450686.png)

在`Zookeeper`这个类中，把一些用户需要和`zookeeper`服务器交互的内容封装成了一个客户端的类。在封装的过程中，需要考虑三个功能：

- 使用这个类的客户怎么快速连接到`zookeeper`服务器？
- 客户怎么创建一个节点？
- 客户怎么查询节点存储的数据？

因此，为了解决这三个问题，在`zookeeperutil`这个类中，实现了三个函数：

- `Start`函数：用来创建一个与Zookeeper服务器连接的会话。在这个函数中，我们要使用框架类来得到`Zookeeper`服务器的IP地址和端口号。然后使用`zookeeper_init`函数进行会话的创建。会话的创建其实是异步的。在创建的时候首先会得到一个句柄，用来操作`zookeeper`。但是句柄的成功返回并不意味着会话创建成功了，所以我们用了了一个信号量阻塞住了创建的线程。等到会话真正创建成功，就会调用全局的回调函数，这时我们才会把信号量加1，唤醒原来的线程。这时就算完全创建成功了。
- `create`函数：用来创建一个zk节点并存储数据。在项目中结点的名字是服务名+方法名，存储的数据是服务方法所在的IP和端口号。在创建的过程中，首先会检查结点是否存在，如果不存在才会创建一个结点。这个函数是给`provider`这个类使用的
- `GetData`函数：用来获取根据结点的路径获取服务方法的IP和端口号。



## 五、异步日志系统 logger类



![image-20220718152552165](https://github.com/Simon-z0404/ImageRepository/blob/main/MPRPC_detail/image-20220718152552165.png)

1. 我们首先设计了一个线程安全的队列。在入队的时候，需要用互斥锁来进行加锁。这把互斥锁使用了C++11中的

`std::lock_guard<std::mutex> lock(mutex)`。主要是用来互斥其他线程（写日志线程拿数据Pop、入队Push），而在离开作用域时可以自动释放这把锁。在写入数据后，就应该通知写日志的线程来拿数据，因此，在入队完成之后，我们还会调用`notify_one`函数把写日志的线程唤醒。

而对于写日志的线程来说，需要用到C++11中的 `std::unique_lock<std::mutex> lock(mutex)`函数。 不用`lock_guard`的原因是`unique_lock`函数提供了锁的`lock`和`unlock`操作，而`lock_guard`没有。如果队列为空的话，我们就使用`m_condvariable.wait(lock)`把这个写日志的线程阻塞休眠，等到入队线程消息的到来。

2. 我们把日志系统设置成为了单例的模式，因为写日志只需要一个专门的对象完成就可以了。在日志系统的构造函数中，首先会开启一个线程，这个线程会不断的Pop出消息，把日志信息写到磁盘IO中。
=======
>>>>>>> 27fc816070053036db3c3d1833ae9b07c75c3f5c
