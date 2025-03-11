

#include "bmw_cic_idrive.hpp"

BMWCIC::~BMWCIC()
{
    if (this->debug)
        delete this->debug;
}

bool BMWCIC::init(ICANBus* canbus){
    CIC_LOG(info)<<"loading plugin...";
    if (this->arbiter) {
        this->aa_handler = this->arbiter->android_auto().handler;
        this->debug = new DebugWindow(*this->arbiter);
        this->canbus = canbus;
        canbus->registerFrameHandler(0x264, [this](QByteArray payload){this->monitorIdriveRotaryStatus(payload);});
        canbus->registerFrameHandler(0x267, [this](QByteArray payload){this->monitorIdriveButtonStatus(payload);});
        // canbus->registerFrameHandler(0x21A, [this](QByteArray payload){this->monitorGearStatus(payload);});
        // canbus->registerFrameHandler(0x1A1, [this](QByteArray payload){this->monitorVehicleSpeed(payload);});
        // canbus->registerFrameHandler(0x0A5, [this](QByteArray payload){this->monitorEngineRPM(payload);});
        canbus->registerFrameHandler(0x273, [this](QByteArray payload){this->monitorCicStatus(payload);});

        // Switch to TV screen on connect.
        // connected event was removed :(
        // auto *oaPage = this->arbiter->layout().openauto_page;
        // connect(oaPage, &OpenAutoPage::connected, this, [this](bool connected){
        connect(this->arbiter, &Arbiter::curr_page_changed, this, [this](Page *page){
            this->switchTVInput();
        });
        CIC_LOG(info)<<"loaded successfully";
        return true;
    }
    else{
        CIC_LOG(error)<<"Failed to get arbiter";
        return false;
    }
}

QList<QWidget *> BMWCIC::widgets()
{
    QList<QWidget *> tabs;
    if (this->debug) {
        tabs.append(this->debug);
    }
    return tabs;
}

// Idrive rotary control
// idrive rotation check, http://www.loopybunny.co.uk/CarPC/can/264.html
// button codes available here: https://github.com/openDsh/aasdk/blob/develop/aasdk_proto/ButtonCodeEnum.proto

void BMWCIC::monitorIdriveRotaryStatus(QByteArray payload){
    if(payload.at(0) == 0xE1 && payload.at(1) == 0xFD && payload.at(5) == 0x1E) {
        this->rotaryPrevPos = this->rotaryPos;
        this->rotaryPos = payload.at(4)*256 + payload.at(3);
        // message.data[4] * 256 + message.data[3]
        if (this->rotaryPos < this->rotaryPrevPos && this->rotaryPrevPos != -1) {
            // rotate counter clockwise
            this->aa_handler->injectButtonPress(aasdk::proto::enums::ButtonCode::SCROLL_WHEEL, openauto::projection::WheelDirection::LEFT);
            // CIC_LOG(info)<<"Rotate counter clockwise";
        } else if (this->rotaryPos > this->rotaryPrevPos && this->rotaryPrevPos != -1) {
            // rotate clockwise
            this->aa_handler->injectButtonPress(aasdk::proto::enums::ButtonCode::SCROLL_WHEEL, openauto::projection::WheelDirection::RIGHT);
            // CIC_LOG(info)<<"Rotate clockwise";
        }
        this->debug->rotaryPos->setText(QString::number(this->rotaryPos));
    }
    if(this->KeyLock && payload.at(0) == 0xE1 && payload.at(1) == 0xFD && payload.at(5) == 0x1E) {
        payload[3] = (uint) 0xFF;
	payload[4] = (uint) 0xFF;
	this->canbus->writeFrame(QCanBusFrame(0x264, payload));
	this->debug->lastKey->setText(QString("Key Lock Enabled"));
    }
}

// Idrive buttons
// and message.data[0] == 0xE1 and message.data[1] == 0x0FD and message.data[2] > c2):
void BMWCIC::monitorIdriveButtonStatus(QByteArray payload){
    // unsigned char j = 0xE1;
    // CIC_LOG(info)<<"got 0x267 frame: "<< QString::number(j, 16).toStdString();
    // qDebug() << "Value : " << hex << j;
    //if(this->cic_fullscreen && payload.at(2) > this->msgCounter &&

   if(payload.at(2) > this->msgCounter &&
        payload.at(0) == 0xE1 && payload.at(1) == 0xFD &&
        (payload.at(4) == 0xDD || payload.at(4) == 0xDE)) {
        if(payload.at(3) == 0x00 && this->lastKey != aasdk::proto::enums::ButtonCode::NONE){
            // Release
            // CIC_LOG(info)<<"Idrive button release";
            this->aa_handler->injectButtonPress(this->lastKey);
            this->lastKey = aasdk::proto::enums::ButtonCode::NONE;
        // } else if(payload.at(3) == 0x01 && payload.at(4) == 0xDE){
        } else if(payload.at(3) == 0x11 && payload.at(4) == 0xDD){
            // UP -> Enter
            // CIC_LOG(info)<<"Up -> Enter";
	        this->lastKey = aasdk::proto::enums::ButtonCode::ENTER;
            this->debug->lastKey->setText(QString("Enter"));
        // } else if(payload.at(3) == 0x11 && payload.at(4) == 0xDD){
        } else if(payload.at(3) == 0x12 && payload.at(4) == 0xDD){
            // UP Hold -> Up
            // CIC_LOG(info)<<"Idrive button UP Hold";
            this->lastKey = aasdk::proto::enums::ButtonCode::UP;
            this->debug->lastKey->setText(QString("Up"));
        // } else if(payload.at(3) == 0x12 && payload.at(4) == 0xDD){
        //     // UP hold
        //     this->lastKey = aasdk::proto::enums::ButtonCode::BACK;
        //     this->debug->lastKey->setText(QString("Up Hold >> Back"));
        } else if(payload.at(3) == 0x41 && payload.at(4) == 0xDD){
            // DOWN
            // CIC_LOG(info)<<"Idrive button Down";
            if (this->lastKey == aasdk::proto::enums::ButtonCode::DOWN) {
                this->lastKey = aasdk::proto::enums::ButtonCode::UP;
                this->debug->lastKey->setText(QString("Up"));
            }
            this->lastKey = aasdk::proto::enums::ButtonCode::DOWN;
            this->debug->lastKey->setText(QString("Down"));
        } else if(payload.at(3) == 0x42 && payload.at(4) == 0xDD){
            // DOWN hold
            this->lastKey = aasdk::proto::enums::ButtonCode::HOME;
            this->debug->lastKey->setText(QString("Down Hold >> HOME"));

        } else if(payload.at(3) == 0x02 && payload.at(4) == 0xDE){
	    // CENTER hold
	    // Enable Key Block
	    if(!this->KeyLock){
		this->debug->KeyLock->setText(QString("Yes"));
	        this->KeyLock = true;
	    } else if (this->KeyLock){
                this->debug->KeyLock->setText(QString("No"));
	        this->KeyLock = false;
	    }
	}
    }

    if(this->KeyLock && payload.at(0) == 0xE1 && payload.at(1) == 0xFD && (payload.at(4) == 0xDE || payload.at(4) == 0xDD) && (payload.at(3) == 0x02 || payload.at(3) == 0x81)) {
        payload[3] = (uint) 0xFF;
        this->canbus->writeFrame(QCanBusFrame(0x267, payload));
	this->debug->lastKey->setText(QString("Key Lock Enabled"));
    }
	
	
    if(payload.at(0) == 0xE1 && payload.at(1) == 0xFD && payload.at(4) == 0xDD &&
        (payload.at(3) == 0x41 || payload.at(3) == 0x11 || payload.at(3) == 0x12 || payload.at(3) == 0x42)) {
        payload[3] = (uint) 0xFF;
        this->canbus->writeFrame(QCanBusFrame(0x267, payload));
    }

    this->msgCounter = payload.at(2);
}

// void BMWCIC::monitorGearStatus(QByteArray payload){
//     if(payload.at(1)%2 == 1 && !this->inReverse){
//         // CIC_LOG(info)<<"Reverse Gear";
//         this->switchTVInput();
//         this->debug->inReverse->setText(QString("Yes"));
// 	    this->inReverse = true;
//         this->arbiter->set_curr_page(3);
//     } else if(payload.at(1)%2 == 0 && this->inReverse){
// 	    // CIC_LOG(info)<<"Not reverse";
//         this->debug->inReverse->setText(QString("No"));
// 	    this->inReverse = false;
//         this->arbiter->set_curr_page(0);
//     }
// }

void BMWCIC::monitorEngineRPM(QByteArray payload){
    int rpm = ((256.0 * (int)payload.at(6)) + (int)payload.at(5)) / 4.0;
    // CIC_LOG(info)<<"RPM: "<<std::to_string(rpm);
    this->debug->rpm->setText(QString::number(rpm));
    // this->arbiter->vehicle_update_data("rpm", rpm);
}

void BMWCIC::monitorVehicleSpeed(QByteArray payload){
    int speed = ((256.0 * (int)payload.at(3)) + (int)payload.at(2)) * 1.609344 / 100.0;
    // CIC_LOG(info)<<"Speed: "<<std::to_string(speed);
    // this->arbiter->vehicle_update_data("speed", speed);
}

void BMWCIC::monitorCicStatus(QByteArray payload){
    this->cic_fullscreen = payload.at(0) == 0x5D;
    // CIC_LOG(info)<<"CIC fullscreen: "<<this->cic_fullscreen;
}

void BMWCIC::switchTVInput(){
    if (!this->cic_fullscreen) {
        // CIC_LOG(info)<<"Switch to TV source";
        this->canbus->writeFrame(QCanBusFrame(0x0A2, QByteArray::fromHex("0080")));
        this->canbus->writeFrame(QCanBusFrame(0x0A2, QByteArray::fromHex("0000")));
    }
}

DebugWindow::DebugWindow(Arbiter &arbiter, QWidget *parent) : QWidget(parent)
{
    this->setObjectName("Dash CIC Debug");

    QLabel* textOne = new QLabel("Last Key", this);
    QLabel* textTwo = new QLabel("KeyLock", this);
    QLabel* textThree = new QLabel("Rotary Pos", this);
    QLabel* textFour = new QLabel("RPM", this);
    QLabel* textFive = new QLabel("In Reverse", this);
    // QLabel* textFour = new QLabel("Message Counter", this);

    lastKey = new QLabel("--", this);
    KeyLock = new QLabel("No", this);
    rotaryPos = new QLabel("--", this);
    rpm = new QLabel("--", this);
    inReverse = new QLabel("No", this);
    // msgCounter = new QLabel("--", this);


    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(textOne);
    layout->addWidget(lastKey);
    layout->addWidget(Session::Forge::br(false));
	
    layout->addWidget(textTwo);
    layout->addWidget(KeyLock);
    layout->addWidget(Session::Forge::br(false));

    layout->addWidget(textThree);
    layout->addWidget(rotaryPos);
    layout->addWidget(Session::Forge::br(false));

    layout->addWidget(textFour);
    layout->addWidget(rpm);
    layout->addWidget(Session::Forge::br(false));
	
    layout->addWidget(textFive);
    layout->addWidget(inReverse);
    layout->addWidget(Session::Forge::br(false));

    // layout->addWidget(textFour);
    // layout->addWidget(msgCounter);
    // layout->addWidget(Session::Forge::br(false));
}
