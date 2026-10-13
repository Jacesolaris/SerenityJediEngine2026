// bg_classmodel.h -- SerenityJediEngine2026 MP: model string -> bot class (g_classmodels.h), shared by game and ui.
#pragma once

// Row of classModels[] (g_classmodels.h) for a model string, -1 when none (the server then uses BCLASS_SABERNOFP).
// Same lookup the server always used (exact model/skin row, exact folder row, substring scan), plus:
// - a character-menu variant folder (models/players/<folder>/charvariant.txt naming the model string it was made from,
//   e.g. stofficer -> stormtrooper/officer) is looked up as that model string;
// - "folder/head_X|torso_X|lower_X" (all three parts the same skin X) also matches a "folder/X" row.
int BG_ClassModelIndex(const char* model);

// botclass for a model string (BCLASS_SABERNOFP when the table has no row), the same answer the server gets.
int BG_ClassModelBotclass(const char* model);

// The model string a character-menu variant folder was made from ("" when the folder is not one).
const char* BG_CharVariantOrigin(const char* folder);

// Models nobody may play (vehicles, props...): the server resets them to the default model.
qboolean BG_ModelBlocked(const char* model);

// bg_skeleton.c: does the model's glm name players/_humanoid/? (a model with neither that nor usable own-skeleton flags
// (BG_ModelSkeletonFlags) is drawn as the default model by the client, so the server resets it too)
qboolean BG_ModelUsesHumanoidGLA(const char* modelName);
