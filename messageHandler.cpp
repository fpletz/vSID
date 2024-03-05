#include "pch.h"

#include "messageHandler.h"

#include <sstream>
#include <iostream> // dev

vsid::MessageHandler::MessageHandler() {}

vsid::MessageHandler::~MessageHandler() { this->closeConsole(); }

void vsid::MessageHandler::writeMessage(std::string sender, std::string msg)
{
	try
	{
		if (this->currentLevel == Level::Debug && sender == "DEBUG")
		{
			std::cout << sender << ": " << msg << '\n';
		}
		else if (/*this->currentLevel != Level::Debug && */sender != "DEBUG") this->msg.push_back(std::pair<std::string, std::string>(sender, msg));
	}
	catch (std::exception& e)
	{
		this->msg.push_back(std::pair<std::string, std::string>("ERROR", e.what()));
	}
	
}

std::pair<std::string, std::string> vsid::MessageHandler::getMessage()
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
}

void vsid::MessageHandler::dropMessage()
{
	this->msg.clear();
}

void vsid::MessageHandler::openConsole()
{
	try
	{
		AllocConsole();
		freopen_s(&this->consoleFile, "CONOUT$", "w", stdout);
	}
	catch (std::exception& e)
	{
		this->msg.push_back(std::pair<std::string, std::string>("ERROR", e.what()));
	}
}

void vsid::MessageHandler::closeConsole()
{
	try
	{
		fclose(this->consoleFile);
		FreeConsole();
	}
	catch (std::exception& e)
	{
		this->msg.push_back(std::pair<std::string, std::string>("ERROR", e.what()));
	}
	
}

int vsid::MessageHandler::getLevel() const
{
	return this->currentLevel;
}

void vsid::MessageHandler::setLevel(std::string lvl)
{
	if (lvl == "DEBUG")
	{
		this->currentLevel = Level::Debug;
		this->openConsole();
	}
	else if (lvl == "INFO")
	{
		this->currentLevel = Level::Info;
		this->closeConsole();
	}
}

//vsid::MessageHandler *vsid::messageHandler = new vsid::MessageHandler();

std::unique_ptr<vsid::MessageHandler> vsid::messageHandler(new vsid::MessageHandler()); // declaration
