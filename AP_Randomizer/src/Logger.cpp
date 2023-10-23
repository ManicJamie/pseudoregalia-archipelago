#pragma once
#include <codecvt>
#include "Unreal/FText.hpp"
#include "DynamicOutput/DynamicOutput.hpp"
#include "Logger.hpp"
#include "Engine.hpp"
#include "Timer.hpp"

namespace Logger {
	using namespace RC::Output;
	using namespace RC::LogLevel;
	using RC::Unreal::FText;

	// Private members
	namespace {
		void PrintToPlayer(std::wstring);
		void PrintToPlayer(std::string);

		std::list<std::wstring> message_queue;
		bool popups_locked;
		// Just over 3 seconds is long enough to ensure only 2 popups can be on screen at once
		const std::chrono::milliseconds popup_delay(3200);
		bool messages_hidden;
		bool messages_muted;
	} // End private members


	void Logger::Log(std::string text, LogType type) {
		// Convert message to wstring and just pass it to the wstring version of Log
		// Consider logging a warning since this is a bit more roundabout than usually necessary
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		Log(converter.from_bytes(text), type);
	}

	void Logger::Log(std::wstring text, LogType type) {
		switch (type) {
		case LogType::Popup: {
			send<LogLevel::Verbose>(L"[APRandomizer] Message: " + text + L"\n");
			PrintToPlayer(text);
			break;
		}

		case LogType::System: {
			send<LogLevel::Verbose>(L"[APRandomizer] System: " + text + L"\n");
			struct PrintToPlayerInfo {
				FText text;
				bool mute_sound;
			};
			std::unique_ptr<FText> new_text(new FText(text));
			std::shared_ptr<void> params(new PrintToPlayerInfo{ *new_text, messages_muted });
			Engine::ExecuteBlueprintFunction(L"BP_APRandomizerInstance_C", L"AP_PrintMiniMessage", params);
			break;
		}

		case LogType::Warning: {
			send<LogLevel::Warning>(L"[APRandomizer] WARNING: " + text + L"\n");
			break;
		}

		case LogType::Error: {
			send<LogLevel::Error>(L"[APRandomizer] ERROR: " + text + L"\n");
			// TODO: functionality to display errors to player
			break;
		}

		default: {
			send<LogLevel::Default>(L"[APRandomizer] : " + text + L"\n");
			break;
		}
		} // End switch
	}

	void Logger::OnTick() {
		if (popups_locked) {
			return;
		}

		if (!message_queue.empty()) {
			struct PrintToPlayerInfo {
				FText text;
				bool mute_sound;
			};
			std::unique_ptr<FText> new_text(new FText(message_queue.front()));
			message_queue.pop_front();
			std::shared_ptr<void> params(new PrintToPlayerInfo{ *new_text, messages_muted });
			Engine::ExecuteBlueprintFunction(L"BP_APRandomizerInstance_C", L"AP_PrintMessage", params);
			Timer::SetBooleanAfterTimer(popup_delay, &popups_locked);
		}
	}

	void Logger::ToggleMessageMute() {
		messages_muted = !messages_muted;
		if (messages_muted) {
			Log(L"Message sounds are now muted.", LogType::System);
		}
		else {
			Log(L"Message sounds are no longer muted.", LogType::System);
		}
	}

	void Logger::ToggleMessageHide() {
		messages_hidden = !messages_hidden;
		if (messages_hidden) {
			message_queue.clear();
			Log(L"Messages are now hidden.", LogType::System);
		}
		else {
			Log(L"Messages are no longer hidden.", LogType::System);
		}
	}


	// Private functions
	namespace {
		void PrintToPlayer(std::wstring message) {
			if (!messages_hidden) {
				message_queue.push_back(message);
			}
		}

		void PrintToPlayer(std::string message) {
			// Convert message to wstring and just pass it to the wstring version of PrintToPlayer
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			std::wstring newwide(message.begin(), message.end());
			PrintToPlayer(converter.from_bytes(message));
		}
	} // End private functions
}