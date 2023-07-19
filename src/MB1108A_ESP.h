#pragma once

#include <ModbusRTU.h>
#include <deque>
#include <vector>
namespace MB11108A
{
    const int LENG_MB = 49;
    uint16_t mb1108_[LENG_MB] = {};
    bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void *data)
    {
        // Serial.printf("MB110-8A result:\t0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
        // for (auto d : mb1108_)
        //     Serial.printf(" %d", d);
        // Serial.println();
        mb1108_[LENG_MB-1] = event;
        return true;
    }
} // namespace name

class MB1108A_ESP
{
    uint8_t netadress_; // адрес устройства в сети Modbus)
    int quantity_;
    const int PERIAD_SAVE_ = 5;                  // периодичность записи показаний в историю, мин
    const int DURATION_ = 120;                   // длительность истории, мин
    const int MAX_SIZE = 120 / PERIAD_SAVE_ + 1; // количество записей в памяти для отображения графика истории
    ModbusRTU *mb11008a_;
    std::deque<std::vector<uint16_t>> deq_{};
    std::vector<uint16_t> v_{};

    void save_deq()
    {
        deq_.push_back(v_);
        // Serial.printf("\n");
        // for (auto i : deq_.back())
        // {
        //     Serial.printf("Wt_save: %d ", i);
        // }
        // Serial.printf("\nSize deq: %d\n", deq_.size());

        if (deq_.size() > MAX_SIZE)
        {
            deq_.pop_front();
        }
    }

public:
    MB1108A_ESP(ModbusRTU *master, int netadress, int quantity) : netadress_(netadress), quantity_(quantity), mb11008a_(master)
    {
    }

    void read()
    {
        mb11008a_->readIreg(netadress_, 0, MB11108A::mb1108_, quantity_ * 6, MB11108A::cbRead);
        while (mb11008a_->slave())
        {
            mb11008a_->task();
            delay(10);
        }
return;
        if (!getErrorMB1108A())
        {
            std::vector<uint16_t> v{};
            for (int i = 0; i < quantity_; i++)
            {
                v.push_back(MB11108A::mb1108_[1 + i * quantity_]);
            }
            std::swap(v_, v);
            if (millis() > 30000)
                save_deq();
        }
    }

    int getErrorMB1108A()
    {
        return MB11108A::mb1108_[MB11108A::LENG_MB-1];
    }

    void setAdress(int adr)
    {
        netadress_ = adr;
    }

    std::deque<std::vector<uint16_t>> *getDeq()
    {
        return &deq_;
    }

    int get_MAX_SIZE()
    {
        return MAX_SIZE;
    }

    uint16_t getData(int index)
    {
        return MB11108A::mb1108_[index];
    }

    std::vector<uint16_t> *getback()
    {
        return &v_;
    }

    bool isEmpty()
    {
        return deq_.empty();
    }

    ~MB1108A_ESP(){};
};