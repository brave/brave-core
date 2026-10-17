namespace sharing_hub {

void SharingHubModel::PopulateFirstPartyActions() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  first_party_action_list_.emplace_back(
      IDC_COPY_URL, l10n_util::GetStringUTF16(IDS_SHARING_HUB_COPY_LINK_LABEL),
      &kCopyOldIcon, "SharingHubDesktop.CopyURLSelected", IDS_LINK_COPIED);
}

}  // namespace sharing_hub
