/*
**
*/
#if defined _taunt_enforcer_included
 #endinput
#endif
#define _taunt_enforcer_included

#include "tf2items.inc"
#include "tf2idb.inc"
#include "clientprefs.inc"
#include "tf2attributes.inc"
//#include "tauntpropfix.inc"

Handle hTauntParticle[1];
bool AboutToTaunt[MAXPLAYERS+6];
//int clientTauntParticle[2048];


public void OnClientConnected(int client)
{
	//clientTauntParticle[client] = 0;
	SetClientCookie(client, hTauntParticle[0], "0");
}

public void OnClientDisconnect(int client)
{
	//clientTauntParticle[client] = 0;
	SetClientCookie(client, hTauntParticle[0], "0");
}

public void TF2_OnConditionRemoved(int client, TFCond cond)
{
	if(IsValidClient(client))
	{
		if(cond == TFCond_Taunting)
		{		
			int particle = MaxClients+1;
			while((particle = FindEntityByClassname(particle, "info_particle_system")) != -1)
			{
				char parent[12];
				GetEntPropString(particle, Prop_Data, "m_iParent", parent, 12);
				ReplaceString(parent, sizeof(parent), "Client", "", false);
				int parentInt = StringToInt(parent);
				if(parentInt == client)
				{
					float origin[3];
					origin[0] = 4350.0;
					origin[1] = -4500.0;
					origin[2] = -2000.0;
					TeleportEntity(particle, origin, NULL_VECTOR, NULL_VECTOR);
					CreateTimer(0.1, tRemoveParticle, particle);
				}
			}
		}
	}
}

public Action tRemoveParticle(Handle timer, any particle)
{
	AcceptEntityInput(particle, "Kill");
}

stock bool IsValidClient(int client){

	if(client > 4096){
		client = EntRefToEntIndex(client);
	}

	if(client < 1 || client > MaxClients)				return false;

	if(!IsClientInGame(client))						return false;

	if(IsFakeClient(client))							return false;
	
	if(GetEntProp(client, Prop_Send, "m_bIsCoaching"))	return false;
	
	return true;

}

int AttachParticle(int ent, char[] particle_name) //attribute won't attach the effect so I have to use this
{
	int particle = CreateEntityByName("info_particle_system");
	if (IsValidEdict(particle))
    {
		float position[3];
		float angles[3];
		GetClientEyeAngles(ent, angles);
		GetEntPropVector(ent, Prop_Send, "m_vecOrigin", position);
		TeleportEntity(particle, position, angles, NULL_VECTOR);
		char Buffer[12];
		Format(Buffer, sizeof(Buffer), "Client%d", ent);
		DispatchKeyValue(ent, "targetname", Buffer);
		SetVariantString(Buffer);
		AcceptEntityInput(particle, "SetParent", ent, particle, 0);
		DispatchKeyValue(particle, "targetname", "tf2particle");
		DispatchKeyValue(particle, "effect_name", particle_name);
		DispatchSpawn(particle);

		if(StrEqual(particle_name, "utaunt_firework_teamcolor_red", false) || StrEqual(particle_name, "utaunt_firework_teamcolor_blue", false))
		{
			SetEntPropVector(particle, Prop_Send, "m_angRotation", angles);
			CreateTimer(2.5, tRepeatParticle, particle, TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
		}
		if(StrEqual(particle_name, "utaunt_lightning_parent", false))
		{
			CreateTimer(0.8, tRepeatParticle, particle, TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
		}
		
		if(StrEqual(particle_name, "utaunt_firework_dragon_parent", false))
		{
			CreateTimer(3.9, tRepeatParticle, particle, TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE);
		}

		ActivateEntity(particle);
		
		AcceptEntityInput(particle, "start");
    }
	return particle;
}

public Action tRepeatParticle(Handle timer, any particle)
{
	if(!IsValidEdict(particle))
	{
		return Plugin_Stop;
	}
	else {
		AcceptEntityInput(particle, "stop");
		CreateTimer(0.1, tTurnOnParticle, particle);
		return Plugin_Continue;
	}
}

public Action tTurnOnParticle(Handle timer, any particle)
{
	if(IsValidEdict(particle))
	{
		AcceptEntityInput(particle, "start");
	}
}

typedef TauntMultipleFilter = function bool(int i_target);

methodmap CTauntEnforcer < Handle {
	public CTauntEnforcer(Handle h_game_config, const char[] s_prop_name = "CTFPlayer::PlayTauntSceneFromItem")
	{
		StartPrepSDKCall(SDKCall_Player);
		PrepSDKCall_SetFromConf(h_game_config, SDKConf_Signature, s_prop_name);
		PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
		PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
		return view_as<CTauntEnforcer>(EndPrepSDKCall());
	}
	
	public bool CBasePlayer_PlayTauntSceneFromItem(int i_client, Address i_pointer)
	{
		return SDKCall(this, i_client, i_pointer);
	}
	
	property Handle m_hBaseTauntItem
	{
		public get() 
		{
			static Handle h_item;
			if (h_item == INVALID_HANDLE)
			{
				h_item = TF2Items_CreateItem(OVERRIDE_ALL|PRESERVE_ATTRIBUTES|FORCE_GENERATION);
				TF2Items_SetClassname(h_item, "tf_wearable_vm");
				TF2Items_SetQuality(h_item, 6);
				TF2Items_SetLevel(h_item, 1);
			}
			return h_item;
		}
	}
	
	public static void MakeCEconViewItem(int i_defindex, float i_particle_id = 0.0)
	{
		//since the getter for ``m_hBaseTauntItem`` won't reference ``this``
		CTauntEnforcer i_null_instance = view_as<CTauntEnforcer>(null);
		Handle h_item = i_null_instance.m_hBaseTauntItem;
		
		TF2Items_SetItemIndex(h_item, i_defindex);
		TF2Items_SetNumAttributes(h_item, 1);
		TF2Items_SetAttribute(h_item, 0, 834, 0.0);
	}
	
	public bool ForceTaunt(int i_client, int i_defindex, int i_particle_id = 0)
	{
		//int particle = 0;
		char cookie[72];
		GetClientCookie(i_client, hTauntParticle[0], cookie, sizeof(cookie));
		int i_ent_index = CreateEntityByName("tf_wearable_vm");
		SetEntProp(i_ent_index, Prop_Send, "m_iItemDefinitionIndex", i_defindex);
		SetEntProp(i_ent_index, Prop_Send, "m_bValidatedAttachedEntity", 1);
		//CTauntEnforcer.MakeCEconViewItem(i_defindex, 0.0);
		//int i_ent_index = TF2Items_GiveNamedItem(i_client, this.m_hBaseTauntItem);
		if (!IsValidEntity(i_ent_index))
		{
			return false;
			//:TODO: Add error checks
		}
		
		SetEntProp(GetEntPropEnt(i_client, Prop_Data, "m_hActiveWeapon"), Prop_Send, "m_bValidatedAttachedEntity", 1);
		
		Address i_pointer = GetEntityAddress(i_ent_index) + view_as<Address>(FindSendPropInfo("CTFWearable", "m_Item"));
		bool b_success = this.CBasePlayer_PlayTauntSceneFromItem(i_client, i_pointer);
		if(!StrEqual(cookie, "0", false) && b_success)
		{
			AttachParticle(i_client, cookie);
		}
		
		AcceptEntityInput(i_ent_index, "Kill");
		return b_success;
	}
	
	public int ForceTauntMultiple(const int[] i_targets, bool[] b_success, int i_nof_targets, int i_defindex, int i_particle_id = 0)
	{
		CTauntEnforcer.MakeCEconViewItem(i_defindex, 0.0);
		// a dummy client will actually hold the taunt, but others can pkay it
		int i_ent_index = TF2Items_GiveNamedItem(i_targets[0], this.m_hBaseTauntItem);
		if (!IsValidEntity(i_ent_index))
		{
			return 0;
			//:TODO: Add error checks
		}
		
		Address i_pointer = GetEntityAddress(i_ent_index) + view_as<Address>(FindSendPropInfo("CTFWearable", "m_Item"));
		
		int i_hits = 0;
		
		for (int i_iter = 0; i_iter < i_nof_targets; i_iter++)
		{
			b_success[i_iter] = this.CBasePlayer_PlayTauntSceneFromItem(i_targets[i_iter], i_pointer);
			if (b_success[i_iter]) { i_hits++; }
		}
		
		AcceptEntityInput(i_ent_index, "Kill");
		return i_hits;
	}
	
	public int ForceTauntMultipleFiltered(const int[] i_targets, bool[] b_success, int i_nof_targets, int i_defindex, TauntMultipleFilter f_filter, int i_particle_id = 0)
	{
		CTauntEnforcer.MakeCEconViewItem(i_defindex, 0.0);
		// a dummy client will actually hold the taunt, but others can pkay it
		int i_ent_index = TF2Items_GiveNamedItem(i_targets[0], this.m_hBaseTauntItem);
		if (!IsValidEntity(i_ent_index))
		{
			return 0;
			//:TODO: Add error checks
		}
		
		Address i_pointer = GetEntityAddress(i_ent_index) + view_as<Address>(FindSendPropInfo("CTFWearable", "m_Item"));
		
		int i_hits = 0;
		bool b_continue;
		
		for (int i_iter = 0; i_iter < i_nof_targets; i_iter++)
		{
			Call_StartFunction(INVALID_HANDLE, f_filter);
			Call_PushCell(i_targets[i_iter]);
			Call_Finish(b_continue);
			if (b_continue)
			{
				b_success[i_iter] = this.CBasePlayer_PlayTauntSceneFromItem(i_targets[i_iter], i_pointer);
			}
			if (b_success[i_iter]) { i_hits++; }
		}
		
		AcceptEntityInput(i_ent_index, "Kill");
		return i_hits;
	}
}