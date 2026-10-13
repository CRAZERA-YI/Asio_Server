# Asio_Server
记录基于Asio库的C++网络编程学习
基于 Boost.Asio 的异步 TCP 服务器

## 功能特性

- **异步 IO**：基于 `boost::asio::io_context` 的事件循环，单线程即可处理大量并发连接
- **粘包处理**：应用层自定义 `[长度][内容]` 协议，正确切分 TCP 字节流
- **发送队列**：每个连接维护独立发送队列，保证消息顺序，避免多线程乱序
- **连接生命周期管理**：使用 `shared_ptr` + `enable_shared_from_this` 延长 Session 生命周期，避免二次析构
- **会话管理**：`CServer` 用 `map` 管理所有连接，支持按 uuid 清理

## 环境要求

- **C++11** 或更高
- **CMake** 3.10+
- **Boost** 1.70+（需要 `system`、`asio`、`uuid` 组件）
- **Linux / macOS / Windows**

### 安装依赖

**Ubuntu / Debian：**

```bash
sudo apt update
sudo apt install build-essential cmake libboost-all-dev
```

**macOS（Homebrew）：**

```bash
brew install cmake boost
```

**Windows：**
- 安装 Visual Studio
- 下载并编译 Boost，或使用 vcpkg：
  ```bash
  vcpkg install boost-asio boost-uuid boost-system
  ```

## 编译

```bash
# 1. 进入项目目录
cd Asio_Server

# 2. 创建 build 目录
mkdir -p build && cd build

# 3. 生成构建文件
cmake ..

# 4. 编译
make -j$(nproc)
```

编译产物在 `bin/` 目录下。

## 运行

```bash
# 启动服务器，监听 10086 端口
./bin/Asio_Server
```

## 通信协议

### 消息格式

采用简化的 TLV 协议：

```
+----------------+------------------+
|  长度 (2字节)   |    消息内容       |
+----------------+------------------+
```

- **长度**：`short` 类型（2 字节），表示消息内容的字节数
- **内容**：实际消息数据

### 示例

发送 `"Hello"`（5 字节）：

```
0x05 0x00  H  e  l  l  o
└─长度─┘  └────内容────┘
```

### 粘包处理

服务器通过以下机制正确处理 TCP 粘包：

1. **先读头部**：收到 2 字节长度
2. **再读消息体**：根据长度读取对应字节数
3. **循环处理**：一次 `read` 可能包含多条消息，用 `while` 循环切分
4. **状态保存**：如果一条消息没收完，用 `_recv_msg_node` 缓存，下次继续

## 项目结构

```
Asio_Server/
├── CMakeLists.txt          # CMake 构建脚本
├── .gitignore              # Git 忽略规则
├── main.cpp                # 程序入口
├── include/
│   ├── CServer.h           # 服务器类
│   └── CSession.h          # 会话类 + MsgNode
└── src/
    ├── CServer.cpp         # 服务器实现
    └── CSession.cpp        # 会话实现
```


