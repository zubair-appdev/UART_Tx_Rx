#include "serialporthandler.h"

serialPortHandler::serialPortHandler(QObject *parent) : QObject(parent)
{
    serial = new QSerialPort;
    connect(serial, &QSerialPort::readyRead, this, &serialPortHandler::readData);

}

serialPortHandler::~serialPortHandler()
{
    delete serial;
}

QStringList serialPortHandler::availablePorts()
{
    QStringList ports;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ports<<info.portName();
    }
    return ports;
}

void serialPortHandler::setPORTNAME(const QString &portName)
{
    buffer.clear();

    if(serial->isOpen())
    {
        serial->close();
    }

    serial->setPortName(portName);
    serial->setBaudRate(921600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);


    if(!serial->open(QIODevice::ReadWrite))
    {
        qDebug()<<"Failed to open port"<<serial->portName();
        emit portOpening("Failed to open port "+serial->portName());
    }
    else
    {
        qDebug() << "Serial port "<<serial->portName()<<" opened successfully at baud rate 921600";
        emit portOpening("Serial port "+serial->portName()+" opened successfully at baud rate 921600");
    }
}

float serialPortHandler::convertBytesToFloat(const QByteArray &data)
{
    if(data.size() != 4)
    {
        qDebug()<<"Insuffient data to convert into float";
    }

    // Assuming little-endian format
    QByteArray floatBytes = data;
    std::reverse(floatBytes.begin(), floatBytes.end()); // Convert to big-endian if needed

    float value;
    memcpy(&value, floatBytes.constData(), sizeof(float));
    return value;
}

quint8 serialPortHandler::chkSum(const QByteArray &data)
{
    // Ensure the QByteArray has at least two bytes (data + checksum)
    if (data.size() < 2) {
        throw std::invalid_argument("Data size must be at least 2 for checksum calculation.");
    }

    // Initialize checksum to 0
    quint8 checksum = 0;

    // Perform XOR for all bytes except the last one
    for (int i = 0; i < data.size() - 1; ++i) {
        checksum ^= static_cast<quint8>(data[i]);
    }

    qDebug()<<hex<<checksum<<"DEBUG_CHKSUM";
    return checksum;
}

void serialPortHandler::readData()
{
    qDebug()<<"------------------------------------------------------------------------------------";
    emit portOpening("------------------------------------------------------------------------------------");
    QByteArray ResponseData;

    // Read data from the serial port
    if (serial->bytesAvailable() == 0) {
        qWarning() << "No bytes available from serial port";
        return;  // Early return if no data is available
    }

    // Create a QMutexLocker to manage the mutex
    QMutexLocker locker(&bufferMutex); // Lock the mutex


    if (serial->bytesAvailable() < std::numeric_limits<int>::max()) {
        buffer.append(serial->readAll()); // Append only if it won't exceed max size
        if (!buffer.isEmpty()) {
                emit dataReceived(); // Signal data has been received
            }
    } else {
        qWarning() << "Attempt to append too much data to QByteArray!";
        return;
    }

    qDebug()<<buffer.toHex()<<" Raw buffer data";
    qDebug()<<buffer.size()<<" :size";
    emit portOpening("Raw readyRead data: "+buffer.toHex());


    //Direct taking msgId from mainWindow
    quint8 msgId = id;
    //powerId to avoid that warning QByteRef calling out of bond error
    quint8 powerId = 0x00;

    //for special cases : large data
    if(msgId == 0xab)
    {
        qDebug()<<"In Special case Pyro Lp line check";
        qDebug() << "Extracted msgId:" <<hex<<msgId;

        if(buffer.size() == 6)
        {
            ResponseData = buffer;
            buffer.clear();
        }
        while(buffer.size() == 1084)
        {
            ResponseData = buffer.left(1084);
            buffer.remove(0,1084);
        }

    }
    else if(msgId == 0x17)
    {
        qDebug()<<"In Special case Get Dau Data Command";
        qDebug() << "Extracted msgId:" <<hex<<msgId;

        if(buffer.size() == 6)
        {
            ResponseData = buffer;
            buffer.clear();
        }
        while(buffer.size() == 533)
        {
            ResponseData = buffer.left(533);
            buffer.remove(0,533);
        }
    }
    else if(msgId == 0x9b)
    {
        qDebug()<<"In Special case Missile on/off";
        qDebug() << "Extracted msgId:" <<hex<<msgId;

        if(buffer.size() == 6)
        {
            ResponseData = buffer;
            buffer.clear();
        }
        while(buffer.size() == 10)
        {
            ResponseData = buffer.left(10);
            buffer.remove(0,10);
        }
    }
    else if(msgId == 0x07)
    {
        qDebug()<<"In Special case Lp OBC RS422";
        qDebug() << "Extracted msgId:" <<hex<<msgId;

        if(buffer.size() == 6)
        {
            ResponseData = buffer;
            buffer.clear();
        }
        while(buffer.size() == 5)
        {
            ResponseData = buffer.left(5);
            buffer.remove(0,5);
        }
    }
    else if(msgId == 0x2a)
    {
        qDebug()<<"In Special case EMA Start/stop";
        qDebug() << "Extracted msgId:" <<hex<<msgId;

        if(buffer.size() == 6)
        {
            ResponseData = buffer;
            buffer.clear();
        }
        while(buffer.size() == 13)
        {
            ResponseData = buffer.left(13);
            buffer.remove(0,13);
        }
    }
    else
    {
        //do nothing
        qDebug()<<"do nothing not a specified size";
    }

    if(ResponseData.size() < 5)
    {
        qDebug()<<"Unexpected response size before"<<ResponseData.size();
    }

    //SPECIAL NOTE : FOR STARTING NEW PROJECT #####################################################
    //1. if condition is enough don't use while loop for checking ex:- here don't need while loop

    /* if(buffer.size() == 6)
        {
            ResponseData = buffer;
            buffer.clear();
        }
        while(buffer.size() == 13)
        {
            ResponseData = buffer.left(13);
            buffer.remove(0,13);
        }
    */

    /* 2. Add CheckSum and Header Validations if not ask embedded team
      Ex : if(buffer.size() == 9
           && static_cast<unsigned char>(buffer[0]) == 0x54
           && static_cast<unsigned char>(buffer[1]) == 0x25
           && static_cast<unsigned char>(buffer[8]) == chkSum(buffer))
    */

    /* 3. use PowerId in switch case only msgId for both if and switch can make problem
     * Ex : if msgId == 0x01 then ResponseData need to be filled
     * but in switch msgId == 0x01 i am asking myVal = ResponseData.mid(3,2) because
     * msgId is common, without ResponseData filling in if condition you are asking to give
     * bytes from ResponseData like above this can cause error like below
     * Using QByteRef with an index pointing outside the valid range of a QByteArray.
     *  The corresponding behavior is deprecated, and will be changed in a future version of Qt.
     * for 5.14 version it is ok, but there will be problem in future.
     */

    // 4. Always ask data type of bytes if 2 bytes whether it is short or unsigned short that's like.

    // 5. just start timer in button() for max 2 sec. : responseTimer and powerId are implemented just use'm.
    // just like this : responseTimer->start(2000); // 2 Sec timer, powerId = 0x01; in if conditions
    /* else if(msgId == 0x08)
    {
        qDebug() << "msgId:" <<hex<<msgId;

        if(buffer.size() == 5
           && static_cast<unsigned char>(buffer[0]) == 0x54
           && static_cast<unsigned char>(buffer[1]) == 0x04
           && static_cast<unsigned char>(buffer[2]) == 0x34
           && static_cast<unsigned char>(buffer[4]) == chkSum(buffer))
        {
            powerId = 0x08;
            ResponseData = buffer;
            buffer.clear();
            executeWriteToNotes("T_IN_HI_2 received bytes: "+ResponseData.toHex());

        }
        else
        {
            executeWriteToNotes("Required 5 bytes Received bytes: "+QString::number(buffer.size())
                                     +" "+buffer.toHex());
        }
    }
	 Example packet structure
    void MainWindow::on_pushButton_RS485_clicked()
    {
        // Start the timeout timer
        responseTimer->start(2000); // 2 Sec timer
        
        QByteArray command;
        
        command.append(0x09); //1
        command.append(0x13); //2
        command.append(0xa4); //3
        command.append(0xff); //4
        command.append(0x41); //5
        
        qDebug() << "RS485 cmd sent : " + hexBytes(command);
        writeToNotes("RS485 cmd sent : " + hexBytes(command));
        
        
        emit sendMsgId(0x02);
        serialObj->writeData(command);
    }
    */

    // #############################################################################################

    switch(powerId)
    {
    case 0x9b:
    {



    }
        break;

    case 0x07:
    {


    }
        break;

    case 0xab:
    {




    }
        break;

    case 0x17:
    {

    }
        break;

    case 0x2a:
    {



    }
        break;

    default:
    {
        qDebug() << "Unknown msgId: " <<hex << msgId << " with data: " << ResponseData.toHex();
        emit portOpening("Unknown msgId: " + QString::number(msgId,16)+" with data: "+ResponseData.toHex());
    }
    }

}

void serialPortHandler::recvMsgId(quint8 id)
{
    qDebug() << "Received id:" <<hex<< id;
    this->id = id;
    buffer.clear();
}
