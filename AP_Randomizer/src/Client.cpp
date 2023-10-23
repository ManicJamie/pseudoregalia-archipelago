#pragma once
#include "Archipelago.h"
#include "GameData.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include "Timer.hpp"

namespace Client {
    // Private members
    namespace {
        void ReceiveItem(int64_t, bool);
        void CheckLocation(int64_t);
        bool ConnectionStatusChanged();
        void SetSlotNumber(int);
        void SetSunGreaves(int);
        bool SetDeathLinkTimer(int);
        void ConnectionTimerExpired();

        AP_ConnectionStatus connection_status;
        std::chrono::seconds connection_timer = std::chrono::seconds(15);
        int slot_number;
        int current_death_link_timer;
        const int death_link_timer_max = 400;
    } // End private members


    void Client::Connect(const char* new_ip, const char* new_slot_name, const char* new_password) {
        GameData::Initialize();
        AP_Init(new_ip, "Pseudoregalia", new_slot_name, new_password);
        AP_NetworkVersion version{ 0, 5, 0 };
        AP_SetClientVersion(&version);
        AP_SetItemClearCallback(&GameData::Initialize); // Yes, this calls Initialize twice. I'll fix it when I add save files.
        AP_SetLocationCheckedCallback(&GameData::CheckLocation);
        AP_SetItemRecvCallback(&ReceiveItem);
        AP_SetDeathLinkSupported(true);
        AP_RegisterSlotDataIntCallback("slot_number", &SetSlotNumber);
        // TODO: Figure out a way to generalize this; might require lambdas?
        AP_RegisterSlotDataIntCallback("split_sun_greaves", &SetSunGreaves);
        AP_Start();

        Timer::CallbackAfterTimer(connection_timer, &ConnectionTimerExpired);
        connection_status = AP_GetConnectionStatus();
        std::string connect_message = "Attempting to connect to ";
        connect_message.append(new_ip);
        connect_message += " with name ";
        connect_message.append(new_slot_name);
        Logger::Log(connect_message, Logger::LogType::System);
        // No need to call SyncItems, that will happen through the callback set in AP_SetItemRecvCallback
    }

    void Client::SendCheck(int64_t id, std::wstring current_world) {
        Logger::Log(L"Sending check with id " + std::to_wstring(id));
        AP_SendItem(id);
    }
    
    // Sends game completion flag to Archipelago.
    void Client::CompleteGame() {
        AP_StoryComplete();

        // Send a key to datastorage upon game completion for PopTracker integration
        // I get the slot number by storing it in slot data on generation but I'm pretty sure there's a less dumb way.
        AP_SetServerDataRequest* completion_flag = new AP_SetServerDataRequest();
        AP_DataStorageOperation* operation = new AP_DataStorageOperation();
        int filler_value = 0;
        operation->operation = "add";
        operation->value = &filler_value;
        completion_flag->key = "Pseudoregalia - Player " + std::to_string(slot_number) + " - Game Complete";
        completion_flag->type = AP_DataType::Int;
        completion_flag->want_reply = true;
        completion_flag->operations.push_back(*operation);
        AP_SetServerData(completion_flag);
        delete completion_flag;
        delete operation;
    }

    void Client::PollServer() {
        if (AP_IsMessagePending()) {
            AP_Message* message = AP_GetLatestMessage();
            Logger::Log(message->text, Logger::LogType::Popup);
            AP_ClearLatestMessage();
            // APCpp releases the memory of message
        }

        if (current_death_link_timer > 0) {
            current_death_link_timer--;
            if (current_death_link_timer <= 0) {
                AP_DeathLinkClear();
            }
        }
        if (AP_DeathLinkPending()) {
            if (SetDeathLinkTimer(death_link_timer_max)) {
                Logger::Log(L"Receiving death link");
                Engine::VaporizeGoat();
            }
        }

        if (ConnectionStatusChanged()) {
            if (connection_status == AP_ConnectionStatus::Authenticated) {
                Engine::SpawnCollectibles();
            }
            if (connection_status == AP_ConnectionStatus::ConnectionRefused) {
                Logger::Log(L"The server refused the connection. Please double-check your connection info and client version, and restart the game.", Logger::LogType::System);
            }
        }
    }

    void Client::SendDeathLink() {
        if (SetDeathLinkTimer(death_link_timer_max)) {
            Logger::Log(L"Sending death link");
            AP_DeathLinkSend();
        }
    }


    // Private functions
    namespace {
        void SetSlotNumber(int num) {
            slot_number = num;
        }

        void SetSunGreaves(int is_true) {
            GameData::SetOption("split_sun_greaves", is_true);
            GameData::SetOption("normal_greaves", !is_true);
        }


        void ReceiveItem(int64_t id, bool notify) {
            Logger::Log(L"Receiving item with id " + std::to_wstring(id));
            GameData::ReceiveItem(id);
            Engine::SyncItems();
        }

        bool SetDeathLinkTimer(int new_time) {
            if (current_death_link_timer > 0) {
                return false;
            }
            current_death_link_timer = new_time;
            return true;
        }

        bool ConnectionStatusChanged() {
            if (connection_status != AP_GetConnectionStatus()) {
                connection_status = AP_GetConnectionStatus();
                return true;
            }
            return false;
        }

        // Prints an error if the connection timer expires with no connection having been established.
        void ConnectionTimerExpired() {
            if (AP_GetConnectionStatus() == AP_ConnectionStatus::Disconnected) {
                Logger::Log(L"Could not find the address entered. Please double-check your connection info and restart the game.", Logger::LogType::System);
            }
        }
    } // End private functions
}