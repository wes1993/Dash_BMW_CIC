#include "bmw_f10_idrive.hpp"

bool BmwF10::init(ICANBus* canbus){
    if (this->arbiter) {
        this->debug = new DebugWindow(*this->arbiter);
        F10_LOG(info)<<"loading plugin...";
        canbus->registerFrameHandler(0x264, [this](QByteArray payload){this->monitorIdriveRotaryStatus(payload);});
        canbus->registerFrameHandler(0x267, [this](QByteArray payload){this->monitorIdriveButtonStatus(payload);});
        canbus->registerFrameHandler(0x21A, [this](QByteArray payload){this->monitorGearStatus(payload);});
        F10_LOG(info)<<"loaded successfully";
        return true;
    }
    else{
        F10_LOG(error)<<"Failed to get arbiter";
        return false;
    }
}

QList<QWidget *> BmwF10::widgets()
{
    QList<QWidget *> tabs;
    tabs.append(this->debug);
    return tabs;
}

// Idrive rotary control
// idrive rotation check, http://www.loopybunny.co.uk/CarPC/can/264.html
// button codes available here: https://github.com/openDsh/aasdk/blob/develop/aasdk_proto/ButtonCodeEnum.proto

void BmwF10::monitorIdriveRotaryStatus(QByteArray payload){
    if(payload.at(0) == 0xE1 && payload.at(1) == 0xFD && payload.at(5) == 0x1E) {
        this->rotaryPrevPos = this->rotaryPos;
        this->rotaryPos = payload.at(4)*256 + payload.at(3);
        // message.data[4] * 256 + message.data[3]
        if (this->rotaryPos < this->rotaryPrevPos && this->rotaryPrevPos != -1) {
            // rotate counter clockwise
            this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::SCROLL_WHEEL, openauto::projection::WheelDirection::LEFT);
            // F10_LOG(info)<<"Rotate counter clockwise";
        } else if (this->rotaryPos > this->rotaryPrevPos && this->rotaryPrevPos != -1) {
            // rotate clockwise
            this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::SCROLL_WHEEL, openauto::projection::WheelDirection::RIGHT);
            // F10_LOG(info)<<"Rotate clockwise";
        }
        // this->debug->rotaryPrevPos->setText(QString::number((uint16_t)this->rotaryPrevPos));
        // this->debug->rotaryPos->setText(QString::number((uint16_t)this->rotaryPos));
    }
}

// Idrive buttons
// and message.data[0] == 0xE1 and message.data[1] == 0x0FD and message.data[2] > c2):
void BmwF10::monitorIdriveButtonStatus(QByteArray payload){
    if(payload.at(0) == 0xE1 && payload.at(1) == 0xFD && payload.at(2) > this->msgCounter) {
        if(payload.at(3) == 0x00 && payload.at(4) == 0xDD && this->lastKey != aasdk::proto::enums::ButtonCode::NONE){
            // Release
            this->arbiter->send_openauto_button_press(this->lastKey);
            // F10_LOG(info)<<"Release";
            // this->debug->lastKey->setText(QString("Release"));
            this->lastKey = aasdk::proto::enums::ButtonCode::NONE;
        } else if(payload.at(3) == 0x01 && payload.at(4) == 0xDE){
            // Enter
            this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::ENTER);
            // F10_LOG(info)<<"Enter";
            // this->debug->lastKey->setText(QString("Enter"));
        } else if(payload.at(3) == 0x11 && payload.at(4) == 0xDD){
            // UP
            this->lastKey = aasdk::proto::enums::ButtonCode::UP;
            // this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::UP);
            // F10_LOG(info)<<"Up";
            // this->debug->lastKey->setText(QString("Up"));
        } else if(payload.at(3) == 0x12 && payload.at(4) == 0xDD){
            // UP hold
            this->lastKey = aasdk::proto::enums::ButtonCode::BACK;
            // this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::BACK);
            // F10_LOG(info)<<"Up Hold >> Back";
            // this->debug->lastKey->setText(QString("Up Hold >> Back"));
        } else if(payload.at(3) == 0x41 && payload.at(4) == 0xDD){
            // DOWN
            this->lastKey = aasdk::proto::enums::ButtonCode::DOWN;
            // this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::DOWN);
            // F10_LOG(info)<<"Down";
            // this->debug->lastKey->setText(QString("Down"));
        } else if(payload.at(3) == 0x42 && payload.at(4) == 0xDD){
            // DOWN hold
            this->lastKey = aasdk::proto::enums::ButtonCode::HOME;
            // this->arbiter->send_openauto_button_press(aasdk::proto::enums::ButtonCode::HOME);
            // F10_LOG(info)<<"Down Hold >> Home";
            // this->debug->lastKey->setText(QString("Down Hold >> HOME"));
        }
        //this->debug->msgCounter->setText(QString::number((uint8_t)this->msgCounter));
    }
}

void BmwF10::monitorGearStatus(QByteArray payload){
    if(payload.at(1)%2 == 1 && !this->inReverse){
        // F10_LOG(info)<<"Reverse Gear";
        // this->debug->inReverse->setText(QString("Yes"));
        this->arbiter->set_curr_page(3);
    } else if(payload.at(1)%2 == 0 && this->inReverse){
        // F10_LOG(info)<<"Not reverse";
        // this->debug->inReverse->setText(QString("No"));
        this->arbiter->set_curr_page(this->arbiter->layout().openauto_page);
    }
}

DebugWindow::DebugWindow(Arbiter &arbiter, QWidget *parent) : QWidget(parent)
{
    this->setObjectName("IdriveDebug");

    QLabel* textOne = new QLabel("In Reverse", this);
    QLabel* textTwo = new QLabel("Rotary Prev Pos", this);
    QLabel* textThree = new QLabel("Rotary Pos", this);
    QLabel* textFour = new QLabel("Message Counter", this);
    QLabel* textFive = new QLabel("Last Key", this);

    inReverse = new QLabel("No", this);
    rotaryPrevPos = new QLabel("--", this);
    rotaryPos = new QLabel("--", this);
    msgCounter = new QLabel("--", this);
    lastKey = new QLabel("--", this);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(textOne);
    layout->addWidget(inReverse);
    layout->addWidget(Session::Forge::br(false));

    layout->addWidget(textTwo);
    layout->addWidget(rotaryPrevPos);
    layout->addWidget(Session::Forge::br(false));

    layout->addWidget(textThree);
    layout->addWidget(rotaryPos);
    layout->addWidget(Session::Forge::br(false));

    layout->addWidget(textFour);
    layout->addWidget(msgCounter);
    layout->addWidget(Session::Forge::br(false));

    layout->addWidget(textFive);
    layout->addWidget(lastKey);
    layout->addWidget(Session::Forge::br(false));
}
