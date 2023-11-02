#include "pch.h"

#include "messageHandler.h"

#include <sstream>
//#include "vSID.h"

//#include <string>

//extern vsid::VSIDPlugin* vsidPlugin;
//extern std::unique_ptr<vsid::VSIDPlugin> vsidPlugin;

vsid::MessageHandler::MessageHandler()
{
	//bool this->debug = false;
}

vsid::MessageHandler::~MessageHandler() {}

void vsid::MessageHandler::writeMessage(std::string sender, std::string msg)
//void vsid::messagehandler::LogMessage(std::string level, std::string msg)
{
	// debug = vsidPlugin->getDebug(); #### while accessing debug member possible access violation (leads to es crash)
	this->msg.push_back(std::pair<std::string, std::string>(sender, msg));
	//vsidPlugin->DisplayUserMessage("vSID", "", logmsg.c_str(), true, true, false, true, false);
}

std::pair<std::string, std::string> vsid::MessageHandler::getMessage()
//void vsid::messagehandler::LogMessage(std::string msg)
{
	if (this->msg.size() > 0)
	{
		std::pair<std::string, std::string> tmp = { this->msg.front().first, this->msg.front().second };
		/*std::ostringstream ss;
		ss << this->msg.front().first << ": " << this->msg.front().second;*/
		this->msg.erase(this->msg.begin());
		return tmp;
	}
	else
	{
		return { "", "" };
	}
	//vsidPlugin->DisplayUserMessage("Message", "", msg.c_str(), true, true, false, false, false);
}

void vsid::MessageHandler::dropMessage()
{
	this->msg.clear();
}

//vsid::MessageHandler *vsid::messageHandler = new vsid::MessageHandler();

std::unique_ptr<vsid::MessageHandler> vsid::messageHandler(new vsid::MessageHandler()); // declaration
