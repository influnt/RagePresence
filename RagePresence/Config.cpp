#include <fmt/format.h>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <spdlog/spdlog.h>
#include <unordered_set>

#include "Tools.h"

using namespace rapidjson;

const char* default_client_id = "791461792382451752";

const char* client_id = default_client_id;
std::unordered_set<std::string> makes = {};

const char* GetDiscordID()
{
	return client_id;
}

bool IsMakeValid(std::string str)
{
	bool valid = makes.count(str);
	spdlog::get("file")->debug("Checking for {0}: {1}", str, valid);
	return valid;
}

void LoadConfig()
{
	// Try to open the file
	std::ifstream ifs("RagePresence.json");
	// If it couldn't be opened, return
	if (!ifs.is_open())
	{
		ShowNotification("The configuration could not be loaded. Make sure that the file exists.");
		return;
	}

	// Wrap the stream
	IStreamWrapper isw(ifs);

	// Try to parse the document
	Document document{};
	document.ParseStream<kParseCommentsFlag>(isw);
	// If there was a parsing error, return
	if (document.HasParseError())
	{
		ShowNotification("Unable to parse the configuration file. Please make sure that the format is valid.");
		std::string error = fmt::format("{0}: {1}", document.GetErrorOffset(), GetParseError_En(document.GetParseError()));
		ShowNotification(error.c_str());
		return;
	}

	// If the document is not an object
	if (!document.IsObject())
	{
		spdlog::get("file")->error("Configuration root object is not a JSON Object");
		ShowNotification("The root of the configuration file is not a JSON Object.");
		return;
	}

	// If there is a Client ID, use it
	if (document.HasMember("client_id"))
	{
		Value& id = document["client_id"];

		if (id.IsString())
		{
			const char* newId = id.GetString();
			spdlog::get("file")->info("Using Client ID from configuration ({0})", newId);
			client_id = newId;
		}
		else
		{
			spdlog::get("file")->warn("Client ID in configuration is not a valid String!");
			spdlog::get("file")->info("Using default Client ID from Lemon ({0})", default_client_id);
			client_id = default_client_id;
		}
	}
	else
	{
		spdlog::get("file")->info("Using default Client ID from Lemon ({0})", default_client_id);
		client_id = default_client_id;
	}

	// If there is a list of makes and is an array, update the existing list
	if (document.HasMember("makes"))
	{
		spdlog::get("file")->debug("Attempting to load list of Vehicle Makes...");
		Value& man = document["makes"];

		if (man.IsArray())
		{
			spdlog::get("file")->debug("Found valid aray, loading...");
			makes.clear();

			for (SizeType i = 0; i < man.Size(); i++)
			{
				Value& newMan = man[i];
				if (newMan.IsString())
				{
					std::string str = newMan.GetString();
					std::transform(str.begin(), str.end(), str.begin(), ::tolower);
					spdlog::get("file")->debug("Added manufacturer: {0}", str);
					makes.insert(str);
				}
			}
		}
	}
}