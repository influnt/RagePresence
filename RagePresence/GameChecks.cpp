#include <discord_rpc.h>
#include <fmt/format.h>
#include <main.h>
#include <natives.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <time.h>

#include "Config.h"
#include "GameChecks.h"
#include "Globals.h"
#include "Tools.h"
#include "Chase.h"

DiscordRichPresence presence;

void CheckForAgency(std::string zone)
{
	bool chaseStateChanged = chaseLastState != IsChaseActive();
	std::string agencyName = GetAgencyName();

	if (chaseStateChanged)
	{
		spdlog::get("file")->debug("Chase state was changed to {0}", chaseStateChanged);
		chaseLastState = IsChaseActive();
		chaseLastAgency = "";
		updateNextTick = true;
	}
	else if (UpdateAgencyName(lastZoneLabel != zone) && agencyName != chaseLastAgency)
	{
		spdlog::get("file")->debug("Agency was changed from '{0}' to '{1}'", chaseLastAgency, agencyName);
		chaseLastState = IsChaseActive();
		chaseLastAgency = agencyName;
		updateNextTick = true;
	}
}

void CheckForZone(std::string zone)
{
	if (lastZoneLabel != zone)
	{
		spdlog::get("file")->debug("Zone was changed from '{0}' to '{1}'", lastZoneLabel, zone);
		lastZoneLabel = zone;
		updateNextTick = true;
	}
}

void CheckForPed(Ped ped)
{
	if (lastPed == ped)
	{
		return;
	}

	lastPed = ped;
	updateNextTick = true;
	spdlog::get("file")->debug("Ped was changed to {0} (Handle: {1})", ENTITY::GET_ENTITY_MODEL(ped), ped);
}

void CheckForVehicle(Vehicle vehicle)
{
	if (lastVehicle == vehicle)
	{
		return;
	}

	lastVehicle = vehicle;
	updateNextTick = true;
	spdlog::get("file")->debug("Vehicle changed to {0}", vehicle);
}

void CheckForMission()
{
	if (missionCustomSet)
	{
		return;
	}

	bool missionRunning = false;

	for (auto const& [script, label] : GetMissionList())
	{
		if (SCRIPT::GET_NUMBER_OF_REFERENCES_OF_SCRIPT_WITH_NAME_HASH_(script) > 0)
		{
			missionRunning = true;

			if (lastMissionHash != script)
			{
				lastMissionHash = script;
				lastMissionLabel = label;
				updateNextTick = true;
				spdlog::get("file")->debug("Mission was changed to {0} (Label: {1})", script, label);
				return;
			}
		}
	}

	if (!missionRunning)
	{
		lastMissionHash = 0;
		lastMissionLabel = "";
	}
}

void UpdatePresenceInfo(Ped ped, Vehicle vehicle, std::string zoneLabel)
{
	// Get the Zone Name and Label as Lowercase
	std::string zoneName = HUD::GET_LABEL_TEXT_(zoneLabel.c_str());
	std::string zoneLower = zoneLabel;
	std::transform(zoneLower.begin(), zoneLower.end(), zoneLower.begin(), ::tolower);
	// And the vehicle information (if any)
	Hash vehicleModel = ENTITY::GET_ENTITY_MODEL(vehicle);
	std::string vehicleLabel = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(vehicleModel);
	std::string vehicleMakeLabel = VEHICLE::GET_MAKE_NAME_FROM_VEHICLE_MODEL_(ENTITY::GET_ENTITY_MODEL(vehicle));
	std::string vehicleMakeName = HUD::GET_LABEL_TEXT_(vehicleMakeLabel.c_str());
	std::transform(vehicleMakeLabel.begin(), vehicleMakeLabel.end(), vehicleMakeLabel.begin(), ::tolower);
	std::string vehicleName = HUD::GET_LABEL_TEXT_(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(vehicleModel));

	// Create a place to store the information.
	std::string details = "";
	std::string state = "";
	std::string smallImage = "";
	std::string smallText = "";

	// Then, set the details of what the player is doing curently
	// Custom Details
	if (detailsCustomSet)
	{
		details = detailsCustomText;
	}
	// Custom Mission
	else if (missionCustomSet)
	{
		details = fmt::format("Playing {0}", missionCustomName);
	}
	// Game Mission
	else if (lastMissionHash != 0 && lastMissionLabel != "")
	{
		details = fmt::format("Playing {0}", HUD::GET_LABEL_TEXT_(lastMissionLabel.c_str()));
	}
	// Freeroaming: Chased by Authorities
	else if (IsChaseActive())
	{
		std::string agency = GetAgencyName();

		if (agency == "")
		{
			details = fmt::format("Being chased by the authorities");
		}
		else
		{
			details = fmt::format("Being chased by {0}", agency);
		}
	}
	// Freeroaming: No Vehicle
	else if (lastVehicle == NULL)
	{
		details = fmt::format("Walking down {0}", zoneName);
	}
	// Freeroaming: With Vehicle
	else
	{
		details = fmt::format("Driving down {0}", zoneName);
	}

	// Ditto, but for the presence state
	if (stateCustomSet)
	{
		state = stateCustomText;
	}
	// On Mission, Custom or Game
	else if (missionCustomSet || lastMissionHash != 0)
	{
		state = "On Mission";
	}
	// Freeroaming
	else
	{
		state = "Freeroaming";
	}

	// If the player is on a vehicle, set the vehicle specific image
	if (lastVehicle != NULL)
	{
		smallText = fmt::format("{0} {1}", vehicleMakeName, vehicleName);

		std::string img = GetMakeImage(vehicleMakeLabel);
		if (img.empty())
		{
			switch (VEHICLE::GET_VEHICLE_CLASS(vehicle))
			{
			case 2:  // SUV
				smallImage = "gen_suv";
				break;
			case 5:  // Sports Classics
				smallImage = "gen_sportsclassics";
				break;
			case 8:  // Motorcycle
				smallImage = "gen_motorcycle";
				break;
			case 12:  // Van
				smallImage = "gen_van";
				break;
			case 13:  // Cycles/Bikes
				smallImage = "gen_bike";
				break;
			case 14:  // Boat
				smallImage = "gen_boat";
				break;
			case 15:  // Helicopter
				smallImage = "gen_helicopter";
				break;
			case 16:  // Plane
				smallImage = "gen_plane";
				break;
			case 18:  // Emergency
				smallImage = "gen_emergency";
				break;
			case 19:  // Military
				smallImage = "gen_military";
				break;
			case 20:  // Commercial
				smallImage = "gen_commercial";
				break;
			case 21:  // Trains
				smallImage = "gen_train";
				break;
			default:
				smallImage = "gen_car";
				break;
			}
		}
		else
		{
			smallImage = fmt::format("man_{0}", img);
		}
	}
	// Otherwise, set the on foot image
	else
	{
		smallText = "On Foot";
		smallImage = "gen_onfoot";
	}

	// If the zone image is valid, use it
	std::string largeImage = "";
	if (IsZoneValid(zoneLower))
	{
		largeImage = fmt::format("zone_{0}", zoneLower);
		presence.largeImageKey = largeImage.c_str();
	}
	// Otherwise, set the ocean image again
	else
	{
		spdlog::get("file")->warn("Zone '{0}' is not valid", zoneLower);
		presence.largeImageKey = "zone_oceana";
	}

	// Set the presence information and update it
	presence.details = details.c_str();
	presence.state = state.c_str();
	presence.smallImageKey = smallImage.c_str();
	presence.smallImageText = smallText.c_str();
	presence.largeImageText = zoneName.c_str();
	Discord_UpdatePresence(&presence);
}

void Init()
{
	// Initialize Discord
	spdlog::get("file")->debug("Initializing Discord Presence...");
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	memset(&presence, 0, sizeof(presence));
	Discord_Initialize(GetDiscordID(), &handlers, 1, "271590");

	// Now, set a random presence just to fill in some gaps
	presence.state = "Loading...";
	presence.details = "Waiting for the Game to load";
	presence.partySize = 1;
	presence.partyMax = 1;
	presence.startTimestamp = time(0);
	presence.largeImageKey = "zone_oceana";
	Discord_UpdatePresence(&presence);
}

void DoGameChecks()
{
	// Load the configuration (if possible) and initialize Discord
	Init();
	LoadConfig();

	// Wait until the playeable character can be controlled
	Player player = PLAYER::PLAYER_ID();
	while (DLC::GET_IS_LOADING_SCREEN_ACTIVE())
	{
		WAIT(0);
	}

	// Generate the cheat hash
	Hash cheatReload = MISC::GET_HASH_KEY("rpreload");
	Hash cheatReconnect = MISC::GET_HASH_KEY("rpreconnect");

	// Now, go ahead and start doing the checks
	while (true)
	{
		player = PLAYER::PLAYER_ID();
		// If the user enters the "rpreload" cheat, reload the configuration
		if (MISC::HAS_CHEAT_STRING_JUST_BEEN_ENTERED_(cheatReload))
		{
			LoadConfig();
			updateNextTick = true;
		}
		// And for "rpreconnect", reconnect to Discord by reinitializing
		if (MISC::HAS_CHEAT_STRING_JUST_BEEN_ENTERED_(cheatReconnect))
		{
			Init();
			updateNextTick = true;
		}

		Ped ped = PLAYER::GET_PLAYER_PED(player);
		Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
		Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, true);
		std::string zone = ZONE::GET_NAME_OF_ZONE(pos.x, pos.y, pos.z);

		CheckForAgency(zone);
		CheckForZone(zone);
		CheckForPed(ped);
		CheckForVehicle(vehicle);
		CheckForMission();

		if (updateNextTick)
		{
			UpdatePresenceInfo(ped, vehicle, zone);
			spdlog::get("file")->debug("Presence updated at {0}", MISC::GET_GAME_TIMER());
			updateNextTick = false;
		}

		WAIT(0);
	}
}
