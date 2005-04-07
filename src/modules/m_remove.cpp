/* Support for a dancer-style /remove command, an alternative to /kick to try and avoid auto-rejoin-on-kick scripts */
/* Written by Om, 25-03-05 */

#include <stdio.h>
#include <string>
#include "users.h"
#include "channels.h"
#include "modules.h"

/* $ModDesc: Provides a /remove command, this is mostly an alternative to /kick, except makes users appear to have parted the channel */

/*	
 * This module supports the use of the +q and +a usermodes, but should work without them too.
 * Usage of the command is restricted to +hoaq, and you cannot remove a user with a "higher" level than yourself.
 * eg: +h can remove +hv and users with no modes. +a can remove +aohv and users with no modes.
*/

Server *Srv;

/* This little function just converts a chanmode character (~ & @ & +) into an integer (5 4 3 2 1) */
int chartolevel(std::string privs)
{
	int level;
	if(privs == "~")
		level = 5;
	else
	if(privs == "&")
		level = 4;
	else
	if(privs == "@")
		level = 3;
	else
	if(privs == "%") {
		level = 2;
	} else {
		level = 1;
	}
	return level;
}
	 
void handle_remove(char **parameters, int pcnt, userrec *user)
{
	/* Look up the user we're meant to be removing from the channel */
	userrec* target = Srv->FindNick(std::string(parameters[0]));
	/* And the channel we're meant to be removing them from */
	chanrec* channel = Srv->FindChannel(std::string(parameters[1]));
	/* And see if the person calling the command has access to use it on the channel */
	std::string privs = Srv->ChanMode(user, channel);
	/* Check what privs the person being removed has */
	std::string targetprivs = Srv->ChanMode(target, channel);
	int tlevel;
	int ulevel;
	int n = 2;
	std::string result;
	
	/* This turns all the parameters after the first two into a single string, so the part reason can be multi-word */
	while (n < pcnt)
	{
		result=result + std::string(" ") + std::string(parameters[n]);
		n++;
	}
	
	/* If the target nick exists... */
	if (target && channel)
	{
		for (int x = 0; x < strlen(parameters[1]); x++)
		{
				if ((parameters[1][0] != '#') || (parameters[1][x] == ' ') || (parameters[1][x] == ','))
				{
					Srv->SendTo(NULL,user,"NOTICE "+std::string(user->nick)+" :*** Invalid characters in channel name");
					return;
				}
		}
		
		/* This is adding support for the +q and +a channel modes, basically if they are enabled, and the remover has them set. */
		/* Then we change the @|%|+ to & if they are +a, or ~ if they are +q */
		if (user->GetExt("cm_protect_"+std::string(channel->name)))
			privs = std::string("&");
		if (user->GetExt("cm_founder_"+std::string(channel->name)))
			privs = std::string("~");
			
		/* Now it's the same idea, except for the target */
		if (target->GetExt("cm_protect_"+std::string(channel->name)))
			targetprivs = std::string("&");
		if (target->GetExt("cm_founder_"+std::string(channel->name)))
			targetprivs = std::string("~");
			
		tlevel = chartolevel(targetprivs);
		ulevel = chartolevel(privs);
		
		/* If the user calling the command is either an admin, owner, operator or a half-operator on the channel */
		if(ulevel > 1)
		{
			/* For now, we'll let everyone remove their level and below, eg ops can remove ops, halfops, voices, and those with no mode (no moders actually are set to 1) */
			if(ulevel >= tlevel)
			{
				Srv->PartUserFromChannel(target,std::string(parameters[1]), "Remove by "+std::string(user->nick)+":"+result);
				Srv->SendTo(NULL,user,"NOTICE "+std::string(channel->name)+" : "+std::string(user->nick)+" removed "+std::string(target->nick)+ " from the channel");
				Srv->SendTo(NULL,target,"NOTICE "+std::string(target->nick)+" :*** "+std::string(user->nick)+" removed you from "+std::string(channel->name)+" with the message:"+std::string(result));
			} else {
				Srv->SendTo(NULL,user,"NOTICE "+std::string(user->nick)+" :*** You do not have access to remove "+std::string(target->nick)+" from the "+std::string(channel->name));
			}
		} else {
			Srv->SendTo(NULL,user,"NOTICE "+std::string(user->nick)+" :*** You do not have access to use /remove on "+std::string(channel->name));
		}
	}
}


class ModuleRemove : public Module
{
 public:
	ModuleRemove()
	{
		Srv = new Server;
		Srv->AddCommand("REMOVE", handle_remove, 0, 3, "m_remove.so");
	}

        virtual void On005Numeric(std::string &output)
        {
                output = output + std::string(" REMOVE");
        }
	
	virtual ~ModuleRemove()
	{
		delete Srv;
	}
	
	virtual Version GetVersion()
	{
		return Version(1,0,0,0);
	}
	
};

// stuff down here is the module-factory stuff. For basic modules you can ignore this.

class ModuleRemoveFactory : public ModuleFactory
{
 public:
	ModuleRemoveFactory()
	{
	}
	
	~ModuleRemoveFactory()
	{
	}
	
	virtual Module * CreateModule()
	{
		return new ModuleRemove;
	}
	
};


extern "C" void * init_module( void )
{
	return new ModuleRemoveFactory;
}

