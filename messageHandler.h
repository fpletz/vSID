#pragma once
#include <string>
#include <memory>
#include <vector>

namespace vsid
{
	/**
	 * @brief Class that manages message output into ES chat
	 * 
	 */
	class MessageHandler
	{
		enum Level
		{
			Debug,
			Info,
			Warning,
			Error
		};

	public:
		MessageHandler();
		virtual ~MessageHandler();

		/**
		 * @brief Writes message to the local storage - this can be used to forward messages to ES chat
		 * 
		 * @param sender front of msg, usually a level of DEBUG, ERROR, WARNING, INFO
		 * @param msg 
		 */
		void writeMessage(std::string sender, std::string msg);
		/**
		 * @brief Retrieve the first message from the local message stack
		 * 
		 * @return
		 */
		std::pair<std::string, std::string> getMessage();
		/**
		 * @brief Deletes the entire message stack
		 * 
		 */
		void dropMessage();
		/**
		 * @brief Opens a console for debugging messages
		 * 
		 */
		void openConsole();
		/**
		 * @brief Closes a opened console
		 * 
		 */
		void closeConsole();
		/**
		 * @brief Get the current message Level
		 * 
		 * @return int 
		 */
		int getLevel() const;
		/**
		 * @brief Set the current message Level
		 * 
		 * @param lvl - "DEBUG" / "INFO"
		 */
		void setLevel(std::string lvl);

	private:
		/**
		 * @brief Local message stack
		 * 
		 */
		std::vector<std::pair<std::string, std::string>> msg;
		FILE* consoleFile = {}; // console file
		Level currentLevel = Level::Debug;
	};

	extern std::unique_ptr< vsid::MessageHandler> messageHandler; // definition - needs to be extern to be accessible from all files
}
