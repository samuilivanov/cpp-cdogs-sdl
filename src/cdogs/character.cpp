/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.

 Copyright (c) 2013-2014, 2016, 2019 Cong Xu
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
#include "character.h"

#include <assert.h>

#include "actors.h"
#include "files.h"
#include "json_utils.h"

#define CHARACTER_VERSION 13

static void CharacterInit(Character *c) {
	memset(c, 0, sizeof *c);

	c->bot = static_cast<CharBot*>(calloc(1, sizeof *c->bot));
	if (c->bot == NULL && sizeof *c->bot > 0) {
		exit(1);
	}
//	CCALLOC(c->bot, sizeof *c->bot);
}
static void CharacterTerminate(Character *c) {
	CFREE(c->Hair);
	CFREE(c->bot);
}

void CharacterStoreInit(CharacterStore *store) {
	memset(store, 0, sizeof *store);
	CArrayInit(&store->OtherChars, sizeof(Character));
	CArrayInit(&store->prisonerIds, sizeof(int));
	CArrayInit(&store->baddieIds, sizeof(int));
	CArrayInit(&store->specialIds, sizeof(int));
}

void CharacterStoreTerminate(CharacterStore *store) {
	CA_FOREACH(Character, c, store->OtherChars)
		CharacterTerminate(c);
	CA_FOREACH_END()
	CArrayTerminate(&store->OtherChars);
	CArrayTerminate(&store->prisonerIds);
	CArrayTerminate(&store->baddieIds);
	CArrayTerminate(&store->specialIds);
	memset(store, 0, sizeof *store);
}

void CharacterStoreResetOthers(CharacterStore *store) {
	CArrayClear(&store->prisonerIds);
	CArrayClear(&store->baddieIds);
	CArrayClear(&store->specialIds);
}

void CharacterLoadJSON(CharacterStore *c, json_t *root, int version) {
	LoadInt(&version, root, "Version");
	json_t *child = json_find_first_label(root, "Characters")->child->child;
	CharacterStoreTerminate(c);
	CharacterStoreInit(c);
	while (child) {
		Character *ch = CharacterStoreAddOther(c);
		char *tmp;

		// Face
		if (version < 13) {
			// Old face names, before face + hair split
			if (version < 7) {
				int face;
				LoadInt(&face, child, "face");

				tmp = static_cast<char*>(malloc(
						strlen(IntCharacterFace(face)) + 1));
				if (tmp == NULL && (strlen(IntCharacterFace(face)) + 1) > 0) {
					exit(1);
				}
				strcpy(tmp, IntCharacterFace(face));

//				CSTRDUP(tmp, IntCharacterFace(face));
			} else {
				tmp = GetString(child, "Class");
			}
			char *face = NULL;
			CharacterOldFaceToHair(tmp, &face, &ch->Hair);
			CFREE(tmp);
			ch->Class = StrCharacterClass(face);
			CFREE(face);
		} else {
			tmp = GetString(child, "Class");
			ch->Class = StrCharacterClass(tmp);
			CFREE(tmp);
			tmp = NULL;
			LoadStr(&ch->Hair, child, "HairType");
			CFREE(tmp);
		}
		CASSERT(ch->Class != NULL, "Cannot load character class");

		// Colours
		if (version < 7) {
			// Old version stored character looks as palette indices
			int skin, arm, body, leg, hair;
			LoadInt(&skin, child, "skin");
			LoadInt(&arm, child, "arm");
			LoadInt(&body, child, "body");
			LoadInt(&leg, child, "leg");
			LoadInt(&hair, child, "hair");
			ConvertCharacterColors(skin, arm, body, leg, hair, &ch->Colors);
		} else {
			LoadColor(&ch->Colors.Skin, child, "Skin");
			LoadColor(&ch->Colors.Arms, child, "Arms");
			LoadColor(&ch->Colors.Body, child, "Body");
			LoadColor(&ch->Colors.Legs, child, "Legs");
			LoadColor(&ch->Colors.Hair, child, "Hair");
		}
		if (version < 12) {
			ConvertHairColors(ch);
		}
		LoadFullInt(&ch->speed, child, "speed");
		tmp = GetString(child, "Gun");
		ch->Gun = StrWeaponClass(tmp);
		CFREE(tmp);
		LoadInt(&ch->maxHealth, child, "maxHealth");
		int flags;
		LoadInt(&flags, child, "flags");
		ch->flags = flags;
		LoadInt(&ch->bot->probabilityToMove, child, "probabilityToMove");
		LoadInt(&ch->bot->probabilityToTrack, child, "probabilityToTrack");
		LoadInt(&ch->bot->probabilityToShoot, child, "probabilityToShoot");
		LoadInt(&ch->bot->actionDelay, child, "actionDelay");
		child = child->next;
	}
}

bool CharacterSave(CharacterStore *s, const char *path) {
	json_t *root = json_new_object();
	AddIntPair(root, "Version", CHARACTER_VERSION);
	bool res = true;

	json_t *charNode = json_new_array();
	CA_FOREACH(Character, c, s->OtherChars)
		json_t *node = json_new_object();
		AddStringPair(node, "Class", c->Class->Name);
		if (c->Hair) {
			AddStringPair(node, "Hair", c->Hair);
		}
		AddColorPair(node, "Skin", c->Colors.Skin);
		AddColorPair(node, "Arms", c->Colors.Arms);
		AddColorPair(node, "Body", c->Colors.Body);
		AddColorPair(node, "Legs", c->Colors.Legs);
		AddColorPair(node, "Hair", c->Colors.Hair);
		AddIntPair(node, "speed", (int) (c->speed * 256));
		json_insert_pair_into_object(node, "Gun",
				json_new_string(c->Gun->name));
		AddIntPair(node, "maxHealth", c->maxHealth);
		AddIntPair(node, "flags", c->flags);
		AddIntPair(node, "probabilityToMove", c->bot->probabilityToMove);
		AddIntPair(node, "probabilityToTrack", c->bot->probabilityToTrack);
		AddIntPair(node, "probabilityToShoot", c->bot->probabilityToShoot);
		AddIntPair(node, "actionDelay", c->bot->actionDelay);
		json_insert_child(charNode, node);
	CA_FOREACH_END()
	json_insert_pair_into_object(root, "Characters", charNode);
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/characters.json", path);
	if (!TrySaveJSONFile(root, buf)) {
		res = false;
		goto bail;
	}

	bail: json_free_value(&root);
	return res;
}

Character* CharacterStoreAddOther(CharacterStore *store) {
	return CharacterStoreInsertOther(store, store->OtherChars.size);
}
Character* CharacterStoreInsertOther(CharacterStore *store, const size_t idx) {
	Character newChar;
	CharacterInit(&newChar);
	CArrayInsert(&store->OtherChars, idx, &newChar);
	return static_cast<Character*>(CArrayGet(&store->OtherChars, idx));
}
void CharacterStoreDeleteOther(CharacterStore *store, int idx) {
	CArrayDelete(&store->OtherChars, idx);
}

void CharacterStoreAddPrisoner(CharacterStore *store, int character) {
	CArrayPushBack(&store->prisonerIds, &character);
}

void CharacterStoreAddBaddie(CharacterStore *store, int character) {
	CArrayPushBack(&store->baddieIds, &character);
}
void CharacterStoreAddSpecial(CharacterStore *store, int character) {
	CArrayPushBack(&store->specialIds, &character);
}
void CharacterStoreDeleteBaddie(CharacterStore *store, int idx) {
	CArrayDelete(&store->baddieIds, idx);
}
void CharacterStoreDeleteSpecial(CharacterStore *store, int idx) {
	CArrayDelete(&store->specialIds, idx);
}

int CharacterStoreGetPrisonerId(const CharacterStore *store, const int i) {
	return *(int*) CArrayGet(&store->prisonerIds, i);
}

int CharacterStoreGetSpecialId(const CharacterStore *store, const int i) {
	return *(int*) CArrayGet(&store->specialIds, i);
}
int CharacterStoreGetRandomBaddieId(const CharacterStore *store) {
	return *(int*) CArrayGet(&store->baddieIds, rand() % store->baddieIds.size);
}
int CharacterStoreGetRandomSpecialId(const CharacterStore *store) {
	return *(int*) CArrayGet(&store->specialIds,
			rand() % store->specialIds.size);
}

bool CharacterIsPrisoner(const CharacterStore *store, const Character *c) {
	for (int i = 0; i < (int) store->prisonerIds.size; i++) {
		const Character *prisoner = static_cast<const Character*>(CArrayGet(
				&store->OtherChars, CharacterStoreGetPrisonerId(store, i)));
		if (prisoner == c) {
			return true;
		}
	}
	return false;
}

int CharacterGetStartingHealth(const Character *c, const bool isNPC) {
	if (isNPC) {
		return MAX(
				(c->maxHealth * ConfigGetInt(&gConfig, "Game.NonPlayerHP"))
						/ 100, 1);
	} else {
		return c->maxHealth;
	}
}

static color_t RandomColor(void);
void CharacterShuffleAppearance(Character *c) {
	// Choose a random character class
	const int numCharClasses = (int) gCharacterClasses.Classes.size
			+ (int) gCharacterClasses.CustomClasses.size;
	const int charClass = rand() % numCharClasses;
	if (charClass < (int) gCharacterClasses.Classes.size) {
		c->Class = static_cast<const CharacterClass*>(CArrayGet(
				&gCharacterClasses.Classes, charClass));
	} else {
		c->Class = static_cast<const CharacterClass*>(CArrayGet(
				&gCharacterClasses.Classes,
				charClass - gCharacterClasses.Classes.size));
	}
	CFREE(c->Hair);
	const char *hairStyleName = *(char**) CArrayGet(&gPicManager.hairstyleNames,
			rand() % gPicManager.hairstyleNames.size);

	c->Hair = static_cast<char*>(malloc(strlen(hairStyleName) + 1));
	if (c->Hair == NULL && (strlen(hairStyleName) + 1) > 0) {
		exit(1);
	}
	strcpy(c->Hair, hairStyleName);
//	CSTRDUP(c->Hair, hairStyleName);
	c->Colors.Skin = RandomColor();
	c->Colors.Arms = RandomColor();
	c->Colors.Body = RandomColor();
	c->Colors.Legs = RandomColor();
	c->Colors.Hair = RandomColor();
}
static color_t RandomColor(void) {
	color_t c;
	c.r = rand() & 255;
	c.g = rand() & 255;
	c.b = rand() & 255;
	c.a = 255;
	return c;
}