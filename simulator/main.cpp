#include "safetylogic.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

enum class Kind{Car,Bicycle,Bus,Truck,ArticulatedTruck};
enum class Lamp{Red,RedYellow,Yellow,Green};
enum class PedStage{Idle,Green,Clearance};
static QString aName(Approach a){static const std::array<QString,4>n{"Nord","Est","Sud","Ovest"};return n.at(int(a));}
static QString mName(Maneuver m){return m==Maneuver::Left?"sinistra":m==Maneuver::Right?"destra":"diritto";}
static RoadUserKind safetyKind(Kind kind){switch(kind){case Kind::Car:return RoadUserKind::Car;case Kind::Bicycle:return RoadUserKind::Bicycle;case Kind::Bus:return RoadUserKind::Bus;case Kind::Truck:return RoadUserKind::Truck;case Kind::ArticulatedTruck:return RoadUserKind::ArticulatedTruck;}return RoadUserKind::Car;}
static QPointF direction(Approach a){switch(a){case Approach::North:return{0,1};case Approach::East:return{-1,0};case Approach::South:return{0,-1};case Approach::West:return{1,0};}return{};}
static QPointF rightOf(QPointF v){return{-v.y(),v.x()};}

struct Track{int id;Approach approach;int lane;Maneuver actual;Kind kind;double progress,speed,measured;bool entered=false,exited=false,logged=false;QColor color;};

class Intersection final:public QWidget{
    Q_OBJECT
public:
    explicit Intersection(QWidget*p=nullptr):QWidget(p){setMinimumSize(790,700);clock.start();connect(&timer,&QTimer::timeout,this,&Intersection::step);timer.start(16);}
    QString phaseText()const{if(emergency)return"ARRESTO DI EMERGENZA";if(fault.active())return"GUASTO MEMORIZZATO · RIPRISTINO MANUALE";if(pedStage==PedStage::Green)return QString("PEDONE VERDE · %1").arg(aName(pedSide));if(pedStage==PedStage::Clearance)return QString("SGOMBERO PEDONALE · %1").arg(aName(pedSide));if(allRed)return occupancy.clear()?"TUTTO ROSSO · attesa richieste":QString("TUTTO ROSSO · sgombero %1 veicoli").arg(occupancy.count());QStringList open;for(int g=0;g<8;g++)if(granted[g])open<<QString("%1-C%2").arg(aName(Approach(g/2))).arg(g%2+1);const QString state=lamp==Lamp::Green?"VERDE":lamp==Lamp::RedYellow?"ROSSO + GIALLO":"GIALLO";return QString("%1 · %2").arg(open.join(" + "),state);}
    int secondsRemaining()const{return std::max(0,int(std::ceil(duration-phaseTime)));}
    int visibleCount()const{return int(tracks.size());}int occupiedCount()const{return occupancy.count();}int anomalyCount()const{return anomalies;}int waitingCount()const{int n=0;for(const auto&t:tracks)if(!t.entered&&t.progress>=.18&&t.progress<.39)n++;return n;}
public slots:
    void setPaused(bool v){paused=v;update();}void setAutoTraffic(bool v){automatic=v;}void setSelectedApproach(int i){selected=Approach(i);}void setSelectedManeuver(int i){selectedM=Maneuver(i);}
    void addCar(){spawnRandom(Kind::Car);}void addBicycle(){spawnRandom(Kind::Bicycle);}void addBus(){spawnRandom(Kind::Bus);}void addTruck(){spawnRandom(Kind::Truck);}
    void requestPedestrian(){pedestrians.request(int(selected));emit eventRaised(QString("PED-CALL · attraversamento %1 memorizzato").arg(aName(selected)));emit statusChanged();}
    void addUnexpected(){
        const int lane=laneFor(selected,selectedM),mask=allowedMask(selected,lane);
        Maneuver actual=selectedM;
        for(int m=0;m<3;m++)if(!(mask&movementBit(Maneuver(m)))){actual=Maneuver(m);break;}
        spawn(selected,selectedM,Kind::Car,true,actual);
    }
    void setEmergency(bool v){emergency=v;if(v){fault.raise();emit eventRaised("E-STOP · guasto memorizzato · tutti i segnali rossi");}else emit eventRaised("E-STOP rilasciato · necessario ripristino manuale");emit statusChanged();update();}
    void resetFault(){if(fault.reset(!emergency,occupancy.clear())){allRed=true;lamp=Lamp::Red;phaseTime=0;duration=1.2;emit eventRaised("RESET MANUALE · ripartenza da tutto rosso");}else emit eventRaised("RESET NEGATO · emergenza attiva o zona occupata");emit statusChanged();update();}
signals:void statusChanged();void eventRaised(const QString&);
protected:
    void paintEvent(QPaintEvent*)override{QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),QColor("#dce8d5"));QPointF c=rect().center();double road=std::min(width(),height())*.43;drawRoad(p,c,road);drawCoverage(p,c,road);drawClearanceArea(p,c,road);drawSignals(p,c,road);drawPedestrian(p,c,road);for(const auto&t:tracks)drawTrack(p,t,c,road);drawState(p,c);drawModeBanner(p);if(paused||emergency||fault.active())drawOverlay(p);}
private:
    QTimer timer;QElapsedTimer clock;std::vector<Track>tracks;OccupancyTracker occupancy;Approach selected=Approach::North;Maneuver selectedM=Maneuver::Straight;Lamp lamp=Lamp::Red;std::array<bool,8>granted{};
    bool allRed=true,paused=false,emergency=false,automatic=true;FaultLatch fault;PedestrianQueue pedestrians;PedStage pedStage=PedStage::Idle;Approach pedSide=Approach::North;double phaseTime=0,duration=0,emptyTime=0,spawnClock=0,gapTime=0,pedSpawnClock=0;int nextId=101,anomalies=0;

    QPainterPath pathFor(const Track&t,QPointF c,double road)const{
        QPointF d=direction(t.approach),r=rightOf(d);double far=std::max(width(),height())*.62,half=road*.5;
        double inOff=t.lane==0?road*.16:road*.36;double outOff=road*.20;
        QPointF start=c-d*far+r*inOff,entry=c-d*half+r*inOff,out=d;if(t.actual==Maneuver::Right)out=r;if(t.actual==Maneuver::Left)out=-r;
        QPointF outRight=rightOf(out),exit=c+out*half+outRight*outOff,finish=c+out*far+outRight*outOff;QPainterPath path(start);path.lineTo(entry);
        if(t.actual==Maneuver::Straight)path.cubicTo(c-d*road*.18+r*inOff,c+d*road*.18+outRight*outOff,exit);
        else if(t.actual==Maneuver::Right)path.quadTo(c-d*road*.28+r*road*.28,exit);
        else path.cubicTo(c-d*road*.08+r*inOff,c+out*road*.08+outRight*outOff,exit);
        path.lineTo(finish);return path;
    }
    bool greenFor(const Track&t)const{return !allRed&&!emergency&&!fault.active()&&granted[int(t.approach)*2+t.lane]&&lamp==Lamp::Green;}
    bool pointInsideSafeZone(const Track&t,double progress)const{
        QPointF c=rect().center();double road=std::min(width(),height())*.43;
        QPointF pos=pathFor(t,c,road).pointAtPercent(std::clamp(progress,0.0,1.0));
        return QRectF(c.x()-road*.43,c.y()-road*.43,road*.86,road*.86).contains(pos);
    }
    bool insideSafeZone(const Track&t,double frontProgress)const{const double rearProgress=std::max(0.0,frontProgress-normalizedRearGap(safetyKind(t.kind)));return pointInsideSafeZone(t,frontProgress)||pointInsideSafeZone(t,rearProgress);}
    double safeExitProgress(const Track&t)const{
        for(double q=std::max(t.progress,.40);q<=1.0;q+=.005)if(!insideSafeZone(t,q))return q;
        return 1.0;
    }
    void step(){const double elapsed=std::max(0.0,clock.restart()/1000.0),motionStep=std::min(.05,elapsed);if(paused)return;if(!emergency&&!fault.active())advancePhase(elapsed);spawnClock+=elapsed;pedSpawnClock+=elapsed;if(automatic&&spawnClock>1.05){spawnClock=0;int roll=QRandomGenerator::global()->bounded(100);auto k=roll<16?Kind::Bicycle:roll<25?Kind::Bus:roll<34?Kind::Truck:roll<38?Kind::ArticulatedTruck:Kind::Car;spawnRandom(k,true);}if(automatic&&pedSpawnClock>11){pedSpawnClock=0;int side=QRandomGenerator::global()->bounded(4);pedestrians.request(side);emit eventRaised(QString("PED-CALL AUTO · %1").arg(aName(Approach(side))));}move(motionStep);emit statusChanged();update();}
    std::array<bool,8> waitingGroups()const{
        std::array<bool,8>w{};for(const auto&t:tracks)if(!t.entered&&t.progress>=.18&&t.progress<.39)w[int(t.approach)*2+t.lane]=true;return w;
    }
    bool hasWaitingGranted()const{auto w=waitingGroups();for(int g=0;g<8;g++)if(w[g]&&granted[g])return true;return false;}
    bool hasWaitingConflicting()const{auto w=waitingGroups();for(int g=0;g<8;g++)if(w[g]&&!granted[g])return true;return false;}
    bool chooseGrants(){
        auto w=waitingGroups();granted.fill(false);int first=-1;double best=-1;
        for(int g=0;g<8;g++)if(w[g])for(const auto&t:tracks)if(int(t.approach)*2+t.lane==g&&!t.entered&&t.progress>best){best=t.progress;first=g;}
        if(first<0)return false;granted[first]=true;
        // Apre insieme tutte le corsie richieste che non creano conflitto.
        for(int g=0;g<8;g++)if(w[g]&&!granted[g]){bool ok=true;for(int h=0;h<8;h++)if(granted[h]&&!laneGroupsCompatible(g,h))ok=false;if(ok)granted[g]=true;}
        return true;
    }
    void choosePedestrianCompatibleGrants(){
        auto waiting=waitingGroups();granted.fill(false);
        for(int group=0;group<8;group++)if(waiting[group]&&laneGroupCompatibleWithCrossing(group,pedSide)){
            bool compatible=true;for(int active=0;active<8;active++)if(granted[active]&&!laneGroupsCompatible(group,active))compatible=false;
            if(compatible)granted[group]=true;
        }
    }
    void advancePhase(double dt){
        phaseTime+=dt;
        if(pedStage==PedStage::Green){
            if(lamp==Lamp::RedYellow&&phaseTime>=SafetyTiming::RedYellowSeconds){lamp=Lamp::Green;emit eventRaised("PED-COMPATIBLE GREEN · soli movimenti verificati non conflittuali");}
            if(phaseTime>=SafetyTiming::PedestrianGreenSeconds){pedStage=PedStage::Clearance;lamp=Lamp::Yellow;phaseTime=0;emit eventRaised("PED-CLEARANCE · nessun nuovo ingresso pedonale o veicolare");}return;
        }
        if(pedStage==PedStage::Clearance){
            if(phaseTime>=SafetyTiming::VehicleYellowSeconds)lamp=Lamp::Red;
            if(phaseTime>=SafetyTiming::PedestrianClearanceSeconds){pedStage=PedStage::Idle;allRed=true;granted.fill(false);lamp=Lamp::Red;phaseTime=0;emptyTime=0;emit eventRaised("PED-OUT · attraversamento confermato libero");}return;
        }
        if(allRed){
            emptyTime=occupancy.clear()?emptyTime+dt:0;
            const double required=occupancy.clear()?SafetyTiming::AllRedSeconds:std::max(2.0,clearanceEstimate());
            if(phaseTime>=required&&emptyTime>=SafetyTiming::AllRedSeconds&&pedestrians.count()>0){auto side=pedestrians.takeNext();pedSide=Approach(side.value());pedStage=PedStage::Green;allRed=false;choosePedestrianCompatibleGrants();lamp=Lamp::RedYellow;phaseTime=0;emit eventRaised(QString("PED-GREEN · %1 · flussi compatibili interbloccati").arg(aName(pedSide)));return;}
            if(phaseTime>=required&&emptyTime>=SafetyTiming::AllRedSeconds&&chooseGrants()){
                allRed=false;lamp=Lamp::RedYellow;phaseTime=0;gapTime=0;duration=SafetyTiming::RedYellowSeconds;
                emit eventRaised("PREPARE · rosso + giallo prima del verde");
            }
            return;
        }
        if(lamp==Lamp::RedYellow){if(phaseTime>=duration){lamp=Lamp::Green;phaseTime=0;gapTime=0;duration=12;emit eventRaised("GREEN · richiesta servita");}return;}
        if(lamp==Lamp::Green){
            gapTime=hasWaitingGranted()?0:gapTime+dt;
            // Aggiunge al verde una nuova corsia solo se compatibile con tutte quelle attive.
            auto w=waitingGroups();for(int g=0;g<8;g++)if(w[g]&&!granted[g]){bool ok=true;for(int h=0;h<8;h++)if(granted[h]&&!laneGroupsCompatible(g,h))ok=false;if(ok){granted[g]=true;emit eventRaised(QString("MERGE · aggiunta %1-C%2 compatibile").arg(aName(Approach(g/2))).arg(g%2+1));}}
            const bool finish=(phaseTime>=SafetyTiming::MinimumVehicleGreenSeconds&&gapTime>=1.2)||(phaseTime>=duration&&hasWaitingConflicting());
            if(finish){lamp=Lamp::Yellow;phaseTime=0;duration=SafetyTiming::VehicleYellowSeconds;}
            return;
        }
        if(phaseTime>=duration){lamp=Lamp::Red;allRed=true;granted.fill(false);phaseTime=0;emptyTime=0;duration=0;emit eventRaised(QString("CLEARANCE · presenti %1").arg(occupancy.count()));}
    }
    double clearanceEstimate()const{double result=2;for(const auto&t:tracks)if(t.entered&&!t.exited){double margin=t.kind==Kind::Bicycle?3.0:2.0;double remaining=std::max(0.0,(safeExitProgress(t)-t.progress)*55);result=std::max(result,clearanceSeconds(remaining,t.measured,2,margin));}return std::min(result,35.0);}
    Maneuver randomAllowedManeuver(Approach a,int lane)const{int mask=allowedMask(a,lane);std::array<Maneuver,3>choices{};int count=0;for(int m=0;m<3;m++)if(mask&(1<<m))choices[count++]=Maneuver(m);return choices[QRandomGenerator::global()->bounded(count)];}
    void spawnRandom(Kind kind,bool allowUnexpected=false){
        Approach a=Approach(QRandomGenerator::global()->bounded(4));
        int lane=QRandomGenerator::global()->bounded(2);
        Maneuver requested=randomAllowedManeuver(a,lane),actual=requested;
        bool unexpected=allowUnexpected&&QRandomGenerator::global()->bounded(100)<9;
        if(unexpected)actual=Maneuver((int(requested)+1+QRandomGenerator::global()->bounded(2))%3);
        spawn(a,requested,kind,unexpected,actual);
    }
    void spawn(Approach a,Maneuver requested,Kind kind,bool unexpected,Maneuver actual){int lane=laneFor(a,requested);for(const auto&t:tracks)if(t.approach==a&&t.lane==lane&&t.progress<.13)return;static const std::array<QColor,6>colors{QColor("#2176ae"),QColor("#ef8354"),QColor("#f2c14e"),QColor("#7356a6"),QColor("#25a18e"),QColor("#d1495b")};double speed=kind==Kind::Bicycle?5+QRandomGenerator::global()->bounded(20):(kind==Kind::Bus||kind==Kind::Truck||kind==Kind::ArticulatedTruck)?7+QRandomGenerator::global()->bounded(24):8+QRandomGenerator::global()->bounded(38);if(!unexpected)actual=requested;tracks.push_back({nextId++,a,lane,actual,kind,0,speed,speed,false,false,false,colors[QRandomGenerator::global()->bounded(6)]});}
    void move(double dt){for(auto&t:tracks){bool stop=t.progress>=.285&&t.progress<.305&&!greenFor(t);if(!stop){double rate=.082*(t.speed/30.0);t.progress+=rate*dt;}if(t.progress>.22&&!t.exited)t.measured=std::max(2.,t.speed+(QRandomGenerator::global()->generateDouble()-.5)*.8);bool inSafeZone=insideSafeZone(t,t.progress);if(!t.entered&&inSafeZone){t.entered=true;occupancy.enter(t.id);QString pace=t.measured<10?"LENTO":t.measured>30?"VELOCE":"normale";emit eventRaised(QString("IN · V%1 %2 · %3 km/h · %4").arg(t.id).arg(t.kind==Kind::Bicycle?"bici":"auto").arg(int(t.measured)).arg(pace));}int mask=allowedMask(t.approach,t.lane);if(t.entered&&!t.logged&&!(mask&movementBit(t.actual))&&t.progress>=.43){t.logged=true;anomalies++;emit eventRaised(QString("W-TRJ-01 · V%1 manovra %2 non prevista · informativo").arg(t.id).arg(mName(t.actual)));}if(t.entered&&!t.exited&&!inSafeZone&&t.progress>.50){t.exited=true;occupancy.leave(t.id);emit eventRaised(QString("OUT · V%1 · bordo safe zone superato · presenti %2").arg(t.id).arg(occupancy.count()));}}tracks.erase(std::remove_if(tracks.begin(),tracks.end(),[](const Track&t){return t.progress>1.03;}),tracks.end());}

    void drawRoad(QPainter&p,QPointF c,double road){p.setPen(Qt::NoPen);p.setBrush(QColor("#343b40"));p.drawRect(QRectF(0,c.y()-road/2,width(),road));p.drawRect(QRectF(c.x()-road/2,0,road,height()));QColor cycle("#a54740");double cw=road*.095;p.fillRect(QRectF(0,c.y()-road/2,width(),cw),cycle);p.fillRect(QRectF(0,c.y()+road/2-cw,width(),cw),cycle);p.fillRect(QRectF(c.x()-road/2,0,cw,height()),cycle);p.fillRect(QRectF(c.x()+road/2-cw,0,cw,height()),cycle);p.setPen(QPen(QColor(240,240,224,150),1.5,Qt::DashLine));for(double f:{-.27,0.,.27}){p.drawLine(QPointF(0,c.y()+road*f),QPointF(width(),c.y()+road*f));p.drawLine(QPointF(c.x()+road*f,0),QPointF(c.x()+road*f,height()));}p.setPen(QPen(QColor(255,255,245,220),4));double h=road*.5;for(int i=-3;i<=3;i++){double q=i*11;p.drawLine(QPointF(c.x()+q,c.y()-h-13),QPointF(c.x()+q,c.y()-h+13));p.drawLine(QPointF(c.x()+q,c.y()+h-13),QPointF(c.x()+q,c.y()+h+13));p.drawLine(QPointF(c.x()-h-13,c.y()+q),QPointF(c.x()-h+13,c.y()+q));p.drawLine(QPointF(c.x()+h-13,c.y()+q),QPointF(c.x()+h+13,c.y()+q));}p.setPen(QPen(QColor(255,255,255,230),4));p.drawLine(QPointF(c.x()-road*.44,c.y()-h),QPointF(c.x()+road*.44,c.y()-h));p.drawLine(QPointF(c.x()-road*.44,c.y()+h),QPointF(c.x()+road*.44,c.y()+h));p.drawLine(QPointF(c.x()-h,c.y()-road*.44),QPointF(c.x()-h,c.y()+road*.44));p.drawLine(QPointF(c.x()+h,c.y()-road*.44),QPointF(c.x()+h,c.y()+road*.44));drawArrows(p,c,road);}
    void drawArrow(QPainter&p,Maneuver m){
        p.drawLine(QPointF(-13,0),QPointF(13,0));p.drawLine(QPointF(13,0),QPointF(6,-5));p.drawLine(QPointF(13,0),QPointF(6,5));
        if(m==Maneuver::Left){p.drawLine(QPointF(0,0),QPointF(0,-10));p.drawLine(QPointF(0,-10),QPointF(-5,-5));}
        if(m==Maneuver::Right){p.drawLine(QPointF(0,0),QPointF(0,10));p.drawLine(QPointF(0,10),QPointF(-5,5));}
    }
    void drawArrows(QPainter&p,QPointF c,double road){p.save();p.setPen(QPen(QColor(255,255,255,225),2.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));for(int ai=0;ai<4;ai++){Approach a=Approach(ai);QPointF d=direction(a),r=rightOf(d);for(int lane=0;lane<2;lane++){double off=lane==0?road*.16:road*.36;QPointF at=c-d*road*.78+r*off;p.save();p.translate(at);p.rotate(std::atan2(d.y(),d.x())*180/M_PI);int mask=allowedMask(a,lane);int count=((mask&1)>0)+((mask&2)>0)+((mask&4)>0),index=0;for(int m=0;m<3;m++)if(mask&(1<<m)){p.save();p.translate(0,(index-(count-1)/2.0)*13);drawArrow(p,Maneuver(m));p.restore();index++;}p.restore();}}p.restore();}
    void drawCoverage(QPainter&p,QPointF c,double road){for(int i=0;i<4;i++){QPointF d=direction(Approach(i)),r=rightOf(d),cam=c-d*road*.88-r*road*.58;QPainterPath cone(cam);cone.lineTo(c-d*road*.56-r*road*.39);cone.lineTo(c+d*road*.20+r*road*.18);cone.closeSubpath();p.fillPath(cone,QColor(38,171,216,12));p.setPen(QPen(QColor(38,171,216,75),1,Qt::DashLine));p.drawPath(cone);p.setPen(Qt::NoPen);p.setBrush(QColor("#102c3a"));p.drawRoundedRect(QRectF(cam.x()-12,cam.y()-8,24,16),4,4);p.setBrush(QColor("#39c7f0"));p.drawEllipse(cam,3,3);QRectF tag(cam.x()-22,cam.y()+11,44,18);p.setBrush(QColor(255,255,255,225));p.drawRoundedRect(tag,5,5);p.setPen(QColor("#173746"));p.drawText(tag,Qt::AlignCenter,QString("CR-%1").arg(i+1));}}
    void drawSignals(QPainter&p,QPointF c,double road){for(int i=0;i<4;i++){Approach a=Approach(i);QPointF d=direction(a),r=rightOf(d);for(int lane=0;lane<2;lane++){int g=i*2+lane;double off=lane==0?road*.16:road*.36;QPointF pos=c-d*road*.60+r*off;p.setPen(Qt::NoPen);p.setBrush(QColor("#111416"));p.drawRoundedRect(QRectF(pos.x()-11,pos.y()-29,22,58),6,6);QColor red="#512029",yellow="#504719",green="#163c28";const bool authorized=!allRed&&!emergency&&!fault.active()&&granted[g];if(authorized&&lamp==Lamp::Green)green="#2de276";else if(authorized&&lamp==Lamp::Yellow)yellow="#ffd447";else if(authorized&&lamp==Lamp::RedYellow){red="#ff4057";yellow="#ffd447";}else red="#ff4057";p.setBrush(red);p.drawEllipse(QPointF(pos.x(),pos.y()-17),6,6);p.setBrush(yellow);p.drawEllipse(pos,6,6);p.setBrush(green);p.drawEllipse(QPointF(pos.x(),pos.y()+17),6,6);p.setPen(QColor("#152127"));p.drawText(QRectF(pos.x()-18,pos.y()+32,36,14),Qt::AlignCenter,QString("C%1").arg(lane+1));}}}
    void drawPedestrian(QPainter&p,QPointF c,double road){p.save();for(int i=0;i<4;i++){Approach a=Approach(i);QPointF d=direction(a),r=rightOf(d),cross=c-d*road*.54;bool active=pedStage==PedStage::Green&&pedSide==a;for(double side:{-1.,1.}){QPointF pos=cross+r*road*.48*side;p.setPen(Qt::NoPen);p.setBrush(QColor("#111416"));p.drawRoundedRect(QRectF(pos.x()-8,pos.y()-15,16,30),4,4);p.setBrush(active?QColor("#193c29"):QColor("#ff4057"));p.drawEllipse(QPointF(pos.x(),pos.y()-7),4.5,4.5);p.setBrush(active?QColor("#2de276"):QColor("#193c29"));p.drawEllipse(QPointF(pos.x(),pos.y()+7),4.5,4.5);}}if(pedStage!=PedStage::Idle){QPointF d=direction(pedSide),r=rightOf(d),cross=c-d*road*.54;double progress=pedStage==PedStage::Green?std::min(1.0,phaseTime/SafetyTiming::PedestrianGreenSeconds):1.0;QPointF pos=cross+r*road*(-.42+.84*progress);p.setPen(QPen(QColor("#f3a712"),3,Qt::SolidLine,Qt::RoundCap));p.setBrush(QColor("#f3a712"));p.drawEllipse(QPointF(pos.x(),pos.y()-10),4,4);p.drawLine(QPointF(pos.x(),pos.y()-5),QPointF(pos.x(),pos.y()+8));p.drawLine(QPointF(pos.x(),pos.y()),QPointF(pos.x()-6,pos.y()+4));p.drawLine(QPointF(pos.x(),pos.y()),QPointF(pos.x()+6,pos.y()+4));p.drawLine(QPointF(pos.x(),pos.y()+8),QPointF(pos.x()-5,pos.y()+15));p.drawLine(QPointF(pos.x(),pos.y()+8),QPointF(pos.x()+5,pos.y()+15));}p.restore();}
    void drawClearanceArea(QPainter&p,QPointF c,double road){if(!allRed||occupancy.clear())return;p.save();p.setPen(QPen(QColor(255,176,32,125),2,Qt::DashLine));p.setBrush(QColor(255,176,32,18));p.drawRoundedRect(QRectF(c.x()-road*.43,c.y()-road*.43,road*.86,road*.86),18,18);for(const auto&t:tracks)if(t.entered&&!t.exited){QPointF pos=pathFor(t,c,road).pointAtPercent(std::clamp(t.progress,0.,1.));p.setPen(QPen(QColor(255,176,32,190),2));p.setBrush(Qt::NoBrush);p.drawEllipse(pos,23,23);}p.restore();}
    void drawTrack(QPainter&p,const Track&t,QPointF c,double road){QPainterPath path=pathFor(t,c,road);double q=std::clamp(t.progress,0.,1.);QPointF pos=path.pointAtPercent(q);double angle=-path.angleAtPercent(q);p.save();p.translate(pos);p.rotate(angle);if(t.kind!=Kind::Bicycle){double length=t.kind==Kind::Car?30:t.kind==Kind::ArticulatedTruck?70:t.kind==Kind::Bus?52:46;p.setPen(QPen(QColor("#17191a"),1.5));p.setBrush(t.color);p.drawRoundedRect(QRectF(-length/2,-8,length,16),5,5);p.setBrush(QColor("#cde8f4"));p.drawRoundedRect(QRectF(length/2-12,-6,10,12),2,2);}else{p.setPen(QPen(QColor("#f7fbfc"),2.2));p.setBrush(Qt::NoBrush);p.drawEllipse(QPointF(-8,0),4,4);p.drawEllipse(QPointF(8,0),4,4);p.drawLine(QPointF(-8,0),QPointF(0,-6));p.drawLine(QPointF(0,-6),QPointF(8,0));p.setBrush(QColor("#f3a712"));p.drawEllipse(QPointF(0,-10),3.5,3.5);}p.rotate(-angle);QRectF tag(-34,-31,68,17);p.setPen(Qt::NoPen);p.setBrush(QColor(250,252,252,235));p.drawRoundedRect(tag,5,5);p.setPen(QColor("#14242c"));QFont f=p.font();f.setPixelSize(10);f.setBold(true);p.setFont(f);QString prefix=t.kind==Kind::Bicycle?"B":t.kind==Kind::Bus?"BUS":t.kind==Kind::Truck?"LKW":t.kind==Kind::ArticulatedTruck?"SZ":"A";p.drawText(tag,Qt::AlignCenter,QString("%1%2 · %3").arg(prefix).arg(t.id).arg(int(t.measured)));p.restore();}
    void drawState(QPainter&p,QPointF c){QRectF b(c.x()-105,c.y()+42,210,70);QColor accent=occupancy.clear()?QColor("#33c985"):QColor("#ffb020");p.setPen(QPen(accent,2));p.setBrush(QColor(12,23,29,235));p.drawRoundedRect(b,11,11);p.setPen(Qt::white);QFont f=p.font();f.setBold(true);f.setPixelSize(14);p.setFont(f);QString text=occupancy.clear()?"✓  ZONA CONFERMATA LIBERA":QString("SGOMBERO PROTETTO\n%1 %2 ancora %3").arg(occupancy.count()).arg(occupancy.count()==1?"veicolo":"veicoli").arg(occupancy.count()==1?"presente":"presenti");p.drawText(b.adjusted(8,5,-8,-5),Qt::AlignCenter,text);}
    void drawModeBanner(QPainter&p){QRectF b(18,18,255,50);p.setPen(Qt::NoPen);p.setBrush(QColor(255,255,255,232));p.drawRoundedRect(b,10,10);p.setPen(QColor("#19313b"));QFont f=p.font();f.setBold(true);f.setPixelSize(13);p.setFont(f);p.drawText(b.adjusted(12,6,-8,-23),Qt::AlignLeft|Qt::AlignVCenter,allRed&&!occupancy.clear()?"SGOMBERO IN CORSO":"CONTROLLO SU DOMANDA");f.setBold(false);f.setPixelSize(11);p.setFont(f);p.setPen(QColor("#5b7078"));p.drawText(b.adjusted(12,25,-8,-4),Qt::AlignLeft|Qt::AlignVCenter,allRed&&!occupancy.clear()?"Nessun nuovo ingresso autorizzato":"Camera-radar e uscite operative");}
    void drawOverlay(QPainter&p){p.fillRect(rect(),QColor(0,0,0,95));p.setPen(Qt::white);QFont f=p.font();f.setBold(true);f.setPixelSize(32);p.setFont(f);QString text=emergency?"ARRESTO DI EMERGENZA\nTUTTO ROSSO":fault.active()?"GUASTO MEMORIZZATO\nRIPRISTINO MANUALE RICHIESTO":"SIMULAZIONE IN PAUSA";p.drawText(rect(),Qt::AlignCenter,text);}
};

class Window final:public QWidget{
public:Window(){setWindowTitle("LumaCross Control · dimostrazione multi-sensore");resize(1240,780);auto*root=new QHBoxLayout(this);root->setContentsMargins(0,0,0,0);root->setSpacing(0);auto*scene=new Intersection;root->addWidget(scene,1);auto*side=new QWidget;side->setFixedWidth(340);side->setStyleSheet("QWidget{background:#f5f7f7;color:#17272e;font-size:13px}QPushButton,QComboBox{min-height:34px;padding:4px 9px;background:white;border:1px solid #cdd7da;border-radius:6px}QPushButton:hover{border-color:#2d93b5;background:#f4fbfd}QComboBox:focus{border-color:#2d93b5}QLabel#heading{font-size:21px;font-weight:800;letter-spacing:.5px}QListWidget{background:#142027;color:#d9e5e9;border:1px solid #273940;border-radius:7px;padding:5px;font-family:Menlo;font-size:10px}QCheckBox{spacing:8px}");auto*panel=new QVBoxLayout(side);panel->setContentsMargins(18,18,18,18);auto*heading=new QLabel("LUMACROSS CONTROL");heading->setObjectName("heading");auto*sub=new QLabel("Controllo su domanda · camera · radar");sub->setStyleSheet("color:#61727a");auto*phase=new QLabel;phase->setWordWrap(true);phase->setStyleSheet("font-size:16px;font-weight:750;padding:12px;background:white;border:1px solid #d5dfe1;border-left:4px solid #2d93b5;border-radius:8px");auto*stats=new QLabel;stats->setStyleSheet("padding:10px;background:#e8eff1;color:#31464f;border-radius:7px");auto*lanes=new QLabel("SCHEMA CORSIE · TUTTI GLI ACCESSI\nC1: ← solo sinistra\nC2: ↑ + → diritto o destra");lanes->setStyleSheet("font-size:12px;padding:9px;background:#fff7e4;color:#4e4228;border:1px solid #e8d49c;border-radius:7px");auto*approach=new QComboBox;approach->addItems({"Nord","Est","Sud","Ovest"});auto*maneuver=new QComboBox;maneuver->addItems({"Sinistra","Diritto","Destra"});auto*car=new QPushButton("+ Automobile casuale");auto*bike=new QPushButton("+ Bicicletta casuale");auto*bus=new QPushButton("+ Autobus");auto*truck=new QPushButton("+ Camion");auto*pedestrian=new QPushButton("+ Chiamata pedonale dal lato scelto");auto*unexpected=new QPushButton("⚠ Crea variazione non prevista");auto*automatic=new QCheckBox("Traffico automatico casuale");automatic->setChecked(true);auto*log=new QListWidget;log->setMinimumHeight(150);auto*pause=new QPushButton("Pausa");pause->setCheckable(true);auto*stop=new QPushButton("ARRESTO D’EMERGENZA");stop->setCheckable(true);stop->setStyleSheet("QPushButton{background:#b42332;color:white;font-weight:800;border:0;border-radius:6px}QPushButton:hover{background:#c72a3b}QPushButton:checked{background:#64101a}");auto*reset=new QPushButton("Ripristino manuale guasto");panel->addWidget(heading);panel->addWidget(sub);panel->addSpacing(10);panel->addWidget(phase);panel->addWidget(stats);panel->addWidget(lanes);panel->addSpacing(6);panel->addWidget(new QLabel("INSERIMENTO MANUALE"));panel->addWidget(car);panel->addWidget(bike);panel->addWidget(bus);panel->addWidget(truck);panel->addWidget(pedestrian);panel->addWidget(new QLabel("CASO SPECIFICO"));panel->addWidget(approach);panel->addWidget(maneuver);panel->addWidget(unexpected);panel->addWidget(automatic);panel->addSpacing(6);panel->addWidget(new QLabel("REGISTRO EVENTI"));panel->addWidget(log,1);panel->addWidget(pause);panel->addWidget(stop);panel->addWidget(reset);root->addWidget(side);
        connect(approach,&QComboBox::currentIndexChanged,scene,&Intersection::setSelectedApproach);connect(maneuver,&QComboBox::currentIndexChanged,scene,&Intersection::setSelectedManeuver);connect(car,&QPushButton::clicked,scene,&Intersection::addCar);connect(bike,&QPushButton::clicked,scene,&Intersection::addBicycle);connect(bus,&QPushButton::clicked,scene,&Intersection::addBus);connect(truck,&QPushButton::clicked,scene,&Intersection::addTruck);connect(pedestrian,&QPushButton::clicked,scene,&Intersection::requestPedestrian);connect(unexpected,&QPushButton::clicked,scene,&Intersection::addUnexpected);connect(automatic,&QCheckBox::toggled,scene,&Intersection::setAutoTraffic);connect(pause,&QPushButton::toggled,scene,[=](bool on){scene->setPaused(on);pause->setText(on?"Riprendi":"Pausa");});connect(stop,&QPushButton::toggled,scene,&Intersection::setEmergency);connect(reset,&QPushButton::clicked,scene,&Intersection::resetFault);connect(scene,&Intersection::eventRaised,this,[=](const QString&e){log->insertItem(0,QDateTime::currentDateTime().toString("HH:mm:ss  ")+e);while(log->count()>80)delete log->takeItem(log->count()-1);});connect(scene,&Intersection::statusChanged,this,[=]{phase->setText(scene->phaseText());stats->setText(QString("In attesa: %1\nVeicoli visibili: %2\nZona conflitto: %3\nAnomalie informative: %4\nTelecamere-radar: 4/4 online").arg(scene->waitingCount()).arg(scene->visibleCount()).arg(scene->occupiedCount()).arg(scene->anomalyCount()));});emit scene->statusChanged();}
};
int main(int argc,char*argv[]){QApplication app(argc,argv);QApplication::setStyle("Fusion");Window w;w.show();return app.exec();}
#include "main.moc"
