#pragma once
#include <QtGlobal>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>

enum class CmdType : quint16 { // 改成 quint16，支持更多命令
    None = 0,
    Login = 1,
    HeartBeat = 2,
    MouseInput = 10,
    ScreenFrame = 20
}; //业务命令类型
//TODO:鼠标输入、键盘输入等命令类型可以继续添加

// 2. 协议头结构体
// 注意：校验位通常不放在头里，而是放在整个包的最后，或者头的最后
#pragma pack(push, 1)
struct PacketHeader {
    quint16 magic;      // 0. 包头，比如 0xABCD，用来快速定位包的开始
    quint32 length;     // 1. 包体长度 ('数据' 部分的长度)
    CmdType type;       // 2. 命令类型
};

struct PacketTail {
    quint16 checkSum;   // 4. 校验和 (CRC16)
};
#pragma pack(pop)


//工具类：提供打包和解包的静态函数
class NetworkPacket {
private:
    static const quint16 MAGIC_NUMBER = 0xABCD;// 定义一个魔数常量

public:
	// 打包函数
    static QByteArray pack(CmdType type, const QByteArray& body) {
        // A. 准备头部
        PacketHeader head;
        head.magic = MAGIC_NUMBER;
        head.length = body.size(); // 记录数据长度
        head.type = type;

        // B. 准备缓冲区
        QByteArray packet;
        int totalSize = sizeof(PacketHeader) + body.size() + sizeof(PacketTail);
        packet.reserve(totalSize);//分配足够空间

        // C. 写入头
        packet.append(reinterpret_cast<const char*>(&head), sizeof(PacketHeader));

        // D. 写入数据
        if (!body.isEmpty()) {
            packet.append(body);
        }

        // E. 计算校验和 (计算范围：通常是 Header + Body)
        // Qt 自带 CRC16 
        quint16 crc = qChecksum(packet.constData(), packet.size());//校验和

        // F. 写入校验位 (包尾)
        packet.append(reinterpret_cast<const char*>(&crc), sizeof(quint16));

        return packet;
    }
	// 2.找到包头，检查魔数，计算总长度（头 + 数据 + 尾）
    // ==========================================
    static int totalSize(const QByteArray& buffer) {
        if (buffer.size() < sizeof(PacketHeader)) return 0;

        // 强转头部指针
        const PacketHeader* head = reinterpret_cast<const PacketHeader*>(buffer.constData());

        // 检查魔数
        if (head->magic != 0xABCD) return 0; // 头部不对

        // 返回：头长 + 数据长 + 尾长
        return sizeof(PacketHeader) + head->length + sizeof(PacketTail);
    }
    // 2. 最小包长度 (头 + 尾)
    // ==========================================
    static int minSize() {
        return sizeof(PacketHeader) + sizeof(PacketTail);
    }
    // 3. 校验逻辑 (供解包时调用)
    // ==========================================
    static bool verify(const QByteArray& fullPacket) {
        // 这是一个接收到的完整包 (包含头、体、尾)
        int dataLen = fullPacket.size() - sizeof(PacketTail);

        // 1. 取出包里的校验码 (最后2个字节)
        const char* ptr = fullPacket.constData();
        quint16 receivedCrc = *reinterpret_cast<const quint16*>(ptr + dataLen);

        // 2. 重新计算前面所有数据的 CRC
        quint16 calculatedCrc = qChecksum(fullPacket.constData(), dataLen);

        // 3. 比较
        return receivedCrc == calculatedCrc;
    }
	// 解包函数

    // 解包结构体 (为了方便返回结果)
    struct UnpackResult {
        bool isValid;
        CmdType type;
        QByteArray body;
    };
    // 解包函数 (用于处理业务)
    // ==========================================
    static UnpackResult unpack(const QByteArray& fullPacket) {
        UnpackResult res;
        res.isValid = false;

        // 调用之前的 verify 函数校验 CRC
        if (!verify(fullPacket)) {
            return res;
        }
        const PacketHeader* head = reinterpret_cast<const PacketHeader*>(fullPacket.constData());
        res.type = head->type;
        // 提取中间的数据部分
        res.body = fullPacket.mid(sizeof(PacketHeader), head->length);
        res.isValid = true;

        return res;
    }
};