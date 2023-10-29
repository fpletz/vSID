#pragma once

//#include "vSIDPlugin.h"
//#include "vSID.h"

#include <string>
#include <memory>
#include <vector>

//#include"EuroScopePlugIn.h"

namespace vsid
{
	/**
	 * @brief Class that manages message output into ES chat
	 * 
	 */
	class MessageHandler
	{
	public:
		MessageHandler();
		virtual ~MessageHandler();

		/**
		 * @brief Writes message to the local storage - this can be used to forward messages to ES chat
		 * 
		 * @param level DEBUG, ERROR, INFO
		 * @param msg 
		 */
		void writeMessage(std::string level, std::string msg);
		/**
		 * @brief Retrieve the first message from the local message stack
		 * 
		 * @return
		 */
		std::string getMessage();
		/**
		 * @brief Deletes the entire message stack
		 * 
		 */
		void dropMessage();

	private:
		/**
		 * @brief Local message stack
		 * 
		 */
		std::vector<std::pair<std::string, std::string>> msg;
		bool debug;
	};
	//namespace messagehandler
	//{
	//	/**
	//	 * @brief Logs message in the message field inside ES (tab "vSID")
	//	 * 
	//	 * @param level - DEBUG | INFO | WARNING | ERROR
	//	 * @param msg - the actual msg that is logged / written
	//	 */
	//	void LogMessage(std::string level, std::string msg);
	//	/**
	//	 * @brief Logs message in the message field inside ES (tab "Message")
	//	 * 	 
	//	 * @param msg - the actual msg that is logged / written
	//	 */
	//	void LogMessage(std::string msg);
	//}

	//extern vsid::MessageHandler *messageHandler;

	extern std::unique_ptr< vsid::MessageHandler> messageHandler; // definition - needs to be extern to be accessible from all files
}
