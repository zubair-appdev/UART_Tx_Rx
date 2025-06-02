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

QString serialPortHandler::hexBytesSerial(QByteArray &cmd)
{
    //**************************Visuals*******************
    QString hexOutput = cmd.toHex().toUpper();
    QString formattedHexOutput;

    for (int i = 0; i < hexOutput.size(); i += 2) {
        if (i > 0) {
            formattedHexOutput += " ";
        }
        formattedHexOutput += hexOutput.mid(i, 2);
    }
    return formattedHexOutput;
    //**************************Visuals*******************
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

    if(msgId == 0x01)
    {
        qDebug() << "msgId:" <<hex<<msgId;

        if(buffer.size() == 17
                && static_cast<unsigned char>(buffer[0]) == 0x54
                && static_cast<unsigned char>(buffer[1]) == 0x01
                && static_cast<unsigned char>(buffer[16]) == chkSum(buffer))
        {
            powerId = 0x01;
            ResponseData = buffer;
            buffer.clear();
            //static function of MainWindow doesn't need to create object of class
            executeWriteToNotes("Power Card Data received bytes check: "+ResponseData.toHex());
        }
        else
        {
            executeWriteToNotes("Required 17 bytes Received bytes: "+QString::number(buffer.size())
                                     +" "+buffer.toHex());
        }

    }
    else if(msgId == 0x02)
    {
        qDebug() << "msgId:" <<hex<<msgId;

        if(buffer.size() == 6
                && static_cast<unsigned char>(buffer[0]) == 0x54
                && static_cast<unsigned char>(buffer[1]) == 0x02
                && static_cast<unsigned char>(buffer[2]) == 0x31
                && static_cast<unsigned char>(buffer[5]) == chkSum(buffer))
        {
            powerId = 0x02;
            ResponseData = buffer;
            buffer.clear();
            executeWriteToNotes("DAC_INA_1p1n received bytes: "+ResponseData.toHex());

        }
        else
        {
            executeWriteToNotes("Required 6 bytes Received bytes: "+QString::number(buffer.size())
                                     +" "+buffer.toHex());
        }
    }
    else
    {
        //do nothing
        qDebug()<<"do nothing not a specified size/unknown msgId";
        executeWriteToNotes("Fatal Error 404");
    }


    //SPECIAL NOTE : FOR STARTING NEW PROJECT #####################################################

    // 1. Always ask data type of bytes if 2 bytes whether it is short or unsigned short that's like.

    // 2. mainwindow button (command send) packet structure

    /*

    void MainWindow::on_pushButton_cpcHI_1_clicked()
    {
        // Start the timeout timer
        responseTimer->start(2000); // 2 Sec timer

        QByteArray command;

        command.append(0x47); //1
        command.append(0x04); //2
        command.append(0x31); //3

        quint8 checkSum = calculateChecksum(command);

        //checksum
        command.append(checkSum); //total 4 bytes


        qDebug() << "cpcHi1 cmd sent : " + hexBytes(command);
        writeToNotes("cpcHi1 cmd sent : " + hexBytes(command));


        emit sendMsgId(0x05);
        serialObj->writeData(command);
    }

    */

    // #############################################################################################

    switch(powerId)
    {
    case 0x01:
    {
        if(ResponseData.toHex() == "ff0aff")
        {
            emit portOpening("ACK_01");
        }
        else
        {
            emit portOpening("NACK_01");
        }
    }
        break;

    default:
    {
        qDebug() << "Unknown powerId: " <<hex << powerId << " with data: " << ResponseData.size();
    }

    }

}

void serialPortHandler::recvMsgId(quint8 id)
{
    qDebug() << "Received id:" <<hex<< id;
    this->id = id;
    buffer.clear();
}
