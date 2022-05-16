#include "chrome/browser/sharing_hub/sharing_hub_model.h"

#define PopulateFirstPartyActions PopulateFirstPartyActions_Chromium

#include "src/chrome/browser/sharing_hub/sharing_hub_model.cc"

#undef PopulateFirstPartyActions

void sharing_hub::SharingHubModel::PopulateFirstPartyActions() {
    PopulateFirstPartyActions_Chromium();
}