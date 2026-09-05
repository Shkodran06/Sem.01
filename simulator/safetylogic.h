#pragma once
#include <algorithm>
#include <unordered_set>
#include <array>
#include <optional>
enum class RoadUserKind{Car,Bicycle,Bus,Truck,ArticulatedTruck};
namespace SafetyTiming {
inline constexpr double AllRedSeconds=1.2;
inline constexpr double RedYellowSeconds=0.9;
inline constexpr double PedestrianGreenSeconds=5.0;
inline constexpr double PedestrianClearanceSeconds=13.5;
inline constexpr double MinimumVehicleGreenSeconds=4.0;
inline constexpr double VehicleYellowSeconds=3.0;
}
enum class Approach{North=0,East=1,South=2,West=3};
enum class Maneuver{Left=0,Straight=1,Right=2};
inline int movementBit(Maneuver movement){return 1<<static_cast<int>(movement);}
inline int allowedMask(Approach,int lane){return lane==0?movementBit(Maneuver::Left):movementBit(Maneuver::Straight)|movementBit(Maneuver::Right);}
inline int laneFor(Approach approach,Maneuver maneuver){for(int lane=0;lane<2;lane++)if(allowedMask(approach,lane)&movementBit(maneuver))return lane;return 0;}
inline Approach destinationOf(Approach approach,Maneuver maneuver){const int offset=maneuver==Maneuver::Left?1:maneuver==Maneuver::Straight?2:3;return Approach((static_cast<int>(approach)+offset)%4);}
inline bool movementConflictsWithCrossing(Approach origin,Maneuver maneuver,Approach crossing){return origin==crossing||destinationOf(origin,maneuver)==crossing;}
inline bool movementsCompatible(Approach a,Maneuver ma,Approach b,Maneuver mb){
    if(a==b)return true;
    if(destinationOf(a,ma)==destinationOf(b,mb))return false;
    const int delta=(static_cast<int>(b)-static_cast<int>(a)+4)%4;
    if(delta==2)return ma!=Maneuver::Left&&mb!=Maneuver::Left;
    return ma==Maneuver::Right&&mb==Maneuver::Right;
}
inline bool laneGroupsCompatible(int first,int second){
    const auto a=Approach(first/2),b=Approach(second/2);const int am=allowedMask(a,first%2),bm=allowedMask(b,second%2);
    for(int x=0;x<3;x++)for(int y=0;y<3;y++)if((am&(1<<x))&&(bm&(1<<y))&&!movementsCompatible(a,Maneuver(x),b,Maneuver(y)))return false;
    return true;
}
inline bool laneGroupCompatibleWithCrossing(int group,Approach crossing){
    const auto origin=Approach(group/2);const int mask=allowedMask(origin,group%2);
    for(int movement=0;movement<3;movement++)if((mask&(1<<movement))&&movementConflictsWithCrossing(origin,Maneuver(movement),crossing))return false;
    return true;
}
inline double vehicleLengthMeters(RoadUserKind kind){
    switch(kind){case RoadUserKind::Car:return 4.6;case RoadUserKind::Bicycle:return 1.9;case RoadUserKind::Bus:return 12.0;case RoadUserKind::Truck:return 10.0;case RoadUserKind::ArticulatedTruck:return 18.75;}return 4.6;
}
inline double normalizedRearGap(RoadUserKind kind){
    switch(kind){case RoadUserKind::Car:return .025;case RoadUserKind::Bicycle:return .012;case RoadUserKind::Bus:return .065;case RoadUserKind::Truck:return .058;case RoadUserKind::ArticulatedTruck:return .095;}return .025;
}
class FaultLatch{
public:
    void raise(){latched_=true;}
    bool reset(bool emergencyReleased,bool conditionSafe){if(!emergencyReleased||!conditionSafe)return false;latched_=false;return true;}
    bool active()const{return latched_;}
private:bool latched_=false;
};
class PedestrianQueue{
public:
    void request(int side){if(side>=0&&side<4)waiting_.at(side)=true;}
    bool waiting(int side)const{return side>=0&&side<4&&waiting_.at(side);}
    int count()const{return std::count(waiting_.begin(),waiting_.end(),true);}
    std::optional<int> takeNext(){for(int offset=1;offset<=4;offset++){int side=(last_+offset)%4;if(waiting_.at(side)){waiting_.at(side)=false;last_=side;return side;}}return std::nullopt;}
private:std::array<bool,4>waiting_{};int last_=3;
};
enum class PedestrianStage{Idle,Green,Clearance};
class PedestrianPhase{
public:
    bool start(bool protectedZoneClear){if(stage_!=PedestrianStage::Idle||!protectedZoneClear)return false;stage_=PedestrianStage::Green;elapsed_=0;return true;}
    void advance(double seconds,bool pedestrianStillCrossing=false){
        elapsed_+=std::max(0.0,seconds);
        if(stage_==PedestrianStage::Green&&elapsed_>=SafetyTiming::PedestrianGreenSeconds){stage_=PedestrianStage::Clearance;elapsed_=0;}
        else if(stage_==PedestrianStage::Clearance&&!pedestrianStillCrossing&&elapsed_>=SafetyTiming::PedestrianClearanceSeconds){stage_=PedestrianStage::Idle;elapsed_=0;}
    }
    PedestrianStage stage()const{return stage_;}
    bool blocksVehicleEntry()const{return stage_!=PedestrianStage::Idle;}
private:PedestrianStage stage_=PedestrianStage::Idle;double elapsed_=0;
};
enum class ConstructionSide{Master,Slave};
enum class ConstructionStage{AllRed,Green,Yellow,Fault};
struct ConstructionTiming{double allRed=1.2,minGreen=4.0,maxGreen=9.0,yellow=3.0;};
struct ConstructionInputs{int masterDemand=0,slaveDemand=0;bool corridorOccupied=false,linkOnline=true,emergency=false,resetFault=false;};
struct ConstructionState{ConstructionStage stage=ConstructionStage::AllRed;std::optional<ConstructionSide>active;double elapsed=0;ConstructionSide last=ConstructionSide::Slave;bool faultLatched=false;};
inline ConstructionState advanceConstruction(ConstructionState state,const ConstructionInputs&inputs,const ConstructionTiming&timing,double seconds){
    if(inputs.emergency||!inputs.linkOnline)return{ConstructionStage::Fault,std::nullopt,0,state.last,true};
    if(state.faultLatched||state.stage==ConstructionStage::Fault){if(!inputs.resetFault)return{ConstructionStage::Fault,std::nullopt,state.elapsed,state.last,true};return{ConstructionStage::AllRed,std::nullopt,0,state.last,false};}
    state.elapsed+=std::max(0.0,seconds);
    if(state.stage==ConstructionStage::AllRed){
        if(inputs.corridorOccupied||state.elapsed<timing.allRed)return state;
        const auto other=state.last==ConstructionSide::Master?ConstructionSide::Slave:ConstructionSide::Master;
        const bool otherDemand=other==ConstructionSide::Master?inputs.masterDemand>0:inputs.slaveDemand>0;
        const bool lastDemand=state.last==ConstructionSide::Master?inputs.masterDemand>0:inputs.slaveDemand>0;
        if(!otherDemand&&!lastDemand)return state;state.stage=ConstructionStage::Green;state.active=otherDemand?other:state.last;state.elapsed=0;return state;
    }
    if(state.stage==ConstructionStage::Green){const bool ownDemand=state.active==ConstructionSide::Master?inputs.masterDemand>0:inputs.slaveDemand>0;if(inputs.corridorOccupied||state.elapsed<timing.minGreen||(ownDemand&&state.elapsed<timing.maxGreen))return state;state.stage=ConstructionStage::Yellow;state.elapsed=0;return state;}
    if(state.stage==ConstructionStage::Yellow&&state.elapsed>=timing.yellow){state.stage=ConstructionStage::AllRed;state.last=state.active.value_or(state.last);state.active.reset();state.elapsed=0;}return state;
}
inline bool constructionSafe(const ConstructionState&state){return state.stage==ConstructionStage::Green||state.stage==ConstructionStage::Yellow?state.active.has_value()&&!state.faultLatched:!state.active.has_value();}
class OccupancyTracker {
public:
    bool enter(int id){return active_.insert(id).second;}
    bool leave(int id){return active_.erase(id)==1;}
    int count()const{return static_cast<int>(active_.size());}
    bool clear()const{return active_.empty();}
private: std::unordered_set<int> active_;
};
inline double clearanceSeconds(double meters,double kmh,double minimum,double margin){
    const double speed=std::clamp(kmh,3.0,80.0);
    return std::max(minimum,meters/(speed/3.6)+margin);
}
