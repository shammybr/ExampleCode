// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <BoneNSoul\UI\StyleSet.h>
#include <BoneNSoul\UI\IconStyle.h>
#ifndef Include_Spell_Types
#define Include_Spell_Types
#include "SpellTypes.h"
#endif


struct SpellDatabase {
	
	const FIconStyle* IconStyle = &StyleSet::Get().GetWidgetStyle<FIconStyle>("IconStyle");
	//LoadObject<UPaperFlipbook>(NULL, TEXT("/Game/Sprites/Spells/Fireball.Fireball"), NULL, LOAD_None, NULL)
	 TArray<DebuffData> DebuffList = {
	 	// ID         Debuff Name          Description          Intensity        Duration           Tick Interval        Current Timer    Number of Ticks     Networked
	 	{ BURNING,    "Burning",      "Burns for x Damage",         1.0f,         5.0f,                2.0f,            0,                    0  ,            false}
	 
	 
	 };
	 
	 
	 
	 TArray<FSpellData> Spellbook = {
	 	 //ID        Name         Description    IsProjectile    Base mana cost   Base damage      Damage Type     Debuff Type                       Animation       	                 SpellLocation                    Icon          Tier          Function
	 	{ESpellID::FIREBALL, "Fireball", "FIREBALL",     true,             0,              1,              FIRE,        {DebuffList[0]},    FVector(30.0f, 1.0f, 15.0f)    ,      FVector(-26.0f, 0.0f, 0.0f) ,   IconStyle->FireballIcon,  1  , &SpellEffects::SpawnProjectile}
	 
	 
	 };
	
	 
};
