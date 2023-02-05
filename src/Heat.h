// управление газовым нагревателем
// #include <vector.h>

#pragma once

class Heat
{

private:
    int relay_heat_, gate_valve1_, gate_valve2_, gate_valve3_;

    unsigned long time_power_off_;
    const uint32_t TIME_WAITING_OFF_ = 900000;
    MB11016P_ESP *relay__;
    std::vector<int> queleOn;
    std::vector<int> queleOff;

public:
    Heat(int relay_heat, int gate_valve1, int gate_valve2, int gate_valve3, MB11016P_ESP *mb11016p) : relay_heat_(relay_heat), gate_valve1_(gate_valve1),
                                                                                                       gate_valve2_(gate_valve2), gate_valve3_(gate_valve3)
    {
        relay__ = mb11016p;
    }
    int getValve1()
    {
        return gate_valve1_;
    }
    int getValve2()
    {
        return gate_valve2_;
    }
    int getValve3()
    {
        return gate_valve3_;
    }
    int getSatusHeat()
    {
        return relay__->getRelay(relay_heat_);
    }

    int getStatusRelay(int relay)
    {
        return relay__->getRelay(relay);
    }

    void update()
    {
        if (millis() > time_power_off_ && !queleOff.empty())
        {
            relay__->setOff(queleOff[0]);
            queleOff.clear();
        }
    }
    void setRelay(int relay, int vol)
    {
        if (relay__->getRelay(relay) != vol)
            if (vol)
            {
                queleOn.push_back(relay);
                relay__->setOn(relay_heat_);
                relay__->setOn(relay);
                if (!queleOff.empty())
                {
                    if (relay != queleOff[0])
                        relay__->setOff(queleOff[0]);
                    queleOff.clear();
                }
            }
            else
            {
                if (!queleOn.empty())
                    queleOn.pop_back();
                else
                    return;
                if (queleOn.empty() && queleOff.empty())
                {
                    queleOff.push_back(relay);
                    relay__->setOff(relay_heat_);
                    time_power_off_ = millis() + TIME_WAITING_OFF_;
                    return;
                }
                relay__->setOff(relay);
            }
    }
// для тестирования работы задвижек
    void setTestRelay(int relay, int vol)
    {
        vol? relay__->setOn(relay): relay__->setOff(relay);    
    }

};