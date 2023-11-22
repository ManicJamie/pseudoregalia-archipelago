#pragma once
#include <optional>
#include "boost/algorithm/string.hpp"
#include "UnrealConsole.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include "StringOps.hpp"

namespace UnrealConsole {
	using std::string;
	using std::wstring;
	using RC::Unreal::FText;

	namespace Hashes {
		using StringOps::HashWstring;
		constexpr size_t connect = HashWstring(L"connect");
		constexpr size_t disconnect = HashWstring(L"disconnect");
		constexpr size_t hint = HashWstring(L"hint");
	}

	// Private members
	namespace {
		void ParseConnect(string);
		void ParseMessageOption(string);
		void TryConnect(wstring);
		string GetNextToken(string&);
		string ConvertTcharToString(const TCHAR*);
		string ConvertWstringToString(wstring);

		const char DELIM = ' ';
	} // End private members

	void UnrealConsole::ProcessInput(FText input) {
		wstring command = input.ToString();
		Log(L"AP console input: " + command);
		if (command[0] == *L"/" || command.front() == *L"!") {
			command.erase(0, 1);
			UnrealConsole::ProcessCommand(command);
		}
		else {
			Client::Say(StringOps::ToNarrow(command));
		}
		return;
	}

	void UnrealConsole::ProcessCommand(wstring input) {
		wstring command = input.substr(0, input.find(L' '));
		wstring args = L"";
		if (input.find(L' ') != wstring::npos) {
			args = (input.substr(input.find(L' ') + 1));
		}

		size_t hashed_command = Hashes::HashWstring(command);
		switch (hashed_command) {
		case Hashes::connect:
			TryConnect(args);
			break;
		case Hashes::disconnect:
			// disconnect
			break;
		case Hashes::hint: {
			wstring hint_input = L"!hint ";
			hint_input += args;
			Client::Say(StringOps::ToNarrow(hint_input));
			break;
		}
		default:
			Log(L"Command not recognized: " + input);
			break;
		}
	}

	void UnrealConsole::ProcessCommand(const TCHAR* new_command) {
		// TODO: This should maybe just return something and have a parent check on its return value to decide what to do, 
		// but for now it's not really worth refactoring
		string command = ConvertTcharToString(new_command);
		string first_word = command.substr(0, command.find(DELIM));
		transform(first_word.begin(), first_word.end(), first_word.begin(), tolower); // Convert the first word in the command to lowercase
		Log("AP console command: " + command);

		if (first_word == "connect") {
			if (command.find(DELIM) == string::npos) {
				Log(L"Please provide an ip address, slot name, and (if necessary) password.", LogType::System);
				return;
			}
			command.erase(0, command.find(DELIM) + 1);
			UnrealConsole::ParseConnect(command);
		}

		if (first_word == "disconnect") {
			Client::Disconnect();
		}

		if (first_word == "message" || first_word == "messages") {
			if (command.find(DELIM) == string::npos) {
				Log(L"Please input an option, such as \"mute\" or \"hide\".", LogType::System);
				return;
			}
			command.erase(0, command.find(DELIM) + 1);
			UnrealConsole::ParseMessageOption(GetNextToken(command));
		}
	}


	// Private functions
	namespace {
		void TryConnect(wstring args) {
			using std::optional;

			auto next_token = [&args](wstring::iterator& iter) -> optional<wstring> {
				// Skip whitespace.
				while (*iter == L' ' && iter != args.end()) {
					iter++;
				};
				if (iter == args.end()) {
					return {};
				}

				wstring token;
				wchar_t delim;
				bool arg_in_quotes = (*iter == L'"');
				if (arg_in_quotes) {
					delim = L'"';
					iter++;
				}
				else {
					delim = L' ';
				}
				while (*iter != delim && iter != args.end()) {
					token += *iter;
					iter++;
				}
				// Don't leave the iterator on the last character of the previous token
				if (iter != args.end()) {
					iter++;
				}
				return token;
				};

			wstring::iterator char_iter = args.begin();
			optional<wstring> token;

			wstring uri;
			token = next_token(char_iter);
			if (!token) {
				Log(L"Please provide an ip address, slot name, and (if necessary) password.", LogType::System);
				return;
			}
			uri = token.value();

			wstring slot_name;
			token = next_token(char_iter);
			if (!token) {
				Log(L"Please provide a slot name and (if necessary) password.", LogType::System);
				return;
			}
			slot_name = token.value();

			wstring password;
			token = next_token(char_iter);
			if (!token) {
				password = L"";
			}
			else {
				password = token.value();
			}

			Log(L"Uri:" + uri + L"//Slot name:" + slot_name + L"//Password:" + password);
			Client::Connect(
				StringOps::ToNarrow(uri),
				StringOps::ToNarrow(slot_name),
				StringOps::ToNarrow(password)
			);
		}

		string ConvertTcharToString(const TCHAR* tchars) {
			// Handling strings instead of wstrings here because they need to be narrow eventually to pass to APCpp.
			// There's no character set conversion so if nonlatin unicode characters are entered this will break completely.
			std::wstring wide(tchars);
			string narrow = StringOps::ToNarrow(wide);
			return narrow;
		}

		void ParseConnect(string args) {
			string ip = GetNextToken(args);
			if (ip.empty()) {
				Log("Please provide an ip address, slot name, and (if necessary) password.", LogType::System);
				return;
			}

			string slot_name = GetNextToken(args);
			if (slot_name.empty()) {
				Log("Please provide a slot name and (if necessary) password.", LogType::System);
				return;
			}

			string password = GetNextToken(args);
			Client::Connect(ip, slot_name, password);
		}

		void ParseMessageOption(string option) {
			if (option.empty()) {
				Log("Please input an option, such as \"mute\" or \"hide\".", LogType::System);
				return;
			}
			if (option == "hide" || option == "unhide" || option == "show") {
				Logger::ToggleMessageHide();
			}
			if (option == "mute" || option == "unmute") {
				Logger::ToggleMessageMute();
			}
		}

		string GetNextToken(string& input) {
			// Gets the next space-separated word and removes it from the input string.
			while (input[0] == DELIM) {
				input.erase(input.begin());
			}
			string token;
			if (input[0] == '"') {
				// look for second double quote and take everything between them as one token
				token = input.substr(1, input.find('"', 1) - 1);
				input.erase(0, input.find('"', 1) + 1);
				return token;
			}
			token = input.substr(0, input.find(DELIM));
			input.erase(0, input.find(DELIM));
			return token;
		}
	} // End private functions
}