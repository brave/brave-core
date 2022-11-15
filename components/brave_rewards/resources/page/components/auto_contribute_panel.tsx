/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { getUserType } from '../../shared/lib/user_type'
import { externalWalletFromExtensionData } from '../../shared/lib/external_wallet'

import {
  SettingsPanel,
  PanelHeader,
  PanelItem,
  PanelTable,
  MonthDay,
  ConfigHeader
} from './settings_panel'

import { ModalContribute } from '../../ui/components'
import { ToggleButton } from '../../shared/components/toggle_button'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { PublisherLink } from './publisher_link'
import { TrashIcon } from './icons/trash_icon'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './auto_contribute_panel.style'

const maxTableSize = 5

export function AutoContributePanel () {
  const { getString, getPluralString } = React.useContext(LocaleContext)
  const actions = useActions()

  const data = useRewardsData((state) => ({
    contributionMinTime: state.contributionMinTime,
    contributionMinVisits: state.contributionMinVisits,
    contributionNonVerified: state.contributionNonVerified,
    contributionVideos: state.contributionVideos,
    contributionMonthly: state.contributionMonthly,
    parameters: state.parameters,
    enabledContribute: state.enabledContribute,
    reconcileStamp: state.reconcileStamp,
    autoContributeList: state.autoContributeList,
    excludedList: state.excludedList,
    externalWallet: state.externalWallet,
    externalWalletProviderList: state.externalWalletProviderList,
    userVersion: state.userVersion
  }))

  const [showModal, setShowModal] = React.useState(false)
  const [modalTab, setModalTab] = React.useState(0)
  const [showConfig, setShowConfig] = React.useState(false)
  const [publisherCountText, setPublisherCountText] = React.useState('')

  React.useEffect(() => {
    getPluralString('publisherCountText', data.autoContributeList.length)
      .then(setPublisherCountText)
  }, [data.autoContributeList.length])

  const activityList = data.autoContributeList.sort(
    (a, b) => b.percentage - a.percentage)

  const canShowModal =
    activityList.length > maxTableSize || data.excludedList.length > 0

  const userType = getUserType(
    data.userVersion,
    externalWalletFromExtensionData(data.externalWallet))

  const toggleModal = () => {
    setShowModal(!showModal)
  }

  const onTabChange = () => {
    setModalTab(modalTab === 0 ? 1 : 0)
  }

  const onEnabledChange = (enabled: boolean) => {
    actions.onSettingSave('enabledContribute', enabled)
  }

  const onRestore = () => {
    actions.restorePublishers()
  }

  const settingSelectHandler = (key: string) => {
    return (event: React.FormEvent<HTMLSelectElement>) => {
      actions.onSettingSave(key, Number(event.currentTarget.value) || 0)
    }
  }

  const settingToggleHandler = (key: string) => {
    return (enabled: boolean) => { actions.onSettingSave(key, enabled) }
  }

  function renderTable (maxRows?: number) {
    let rows = activityList
    if (maxRows) {
      rows = rows.slice(0, maxRows)
    }

    return (
      <PanelTable>
        <table data-test-id='auto-contribute-table'>
          <thead>
            <tr>
              <th>{getString('site')}</th>
              <th className='number'>
                {getString('rewardsContributeAttention')}
              </th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {
              rows.map((item, index) => {
                const removePublisher = () => {
                  actions.excludePublisher(item.id)
                }

                return (
                  <tr key={index}>
                    <td data-test-id={`ac_link_${item.publisherKey}`}>
                      <PublisherLink
                        name={item.name}
                        url={item.url}
                        icon={item.favIcon}
                        platform={item.provider}
                        verified={item.status > 0}
                      />
                    </td>
                    <td className='number'>
                      {item.percentage}%
                    </td>
                    <td className='number'>
                      <style.remove>
                        <button onClick={removePublisher}>
                          <TrashIcon />
                        </button>
                      </style.remove>
                    </td>
                  </tr>
                )
              })
            }
          </tbody>
        </table>
        {
          activityList.length === 0 &&
            <div className='empty'>
              {getString('contributionVisitSome')}
            </div>
        }
      </PanelTable>
    )
  }

  function renderMonthlyAmountSelect () {
    return (
      <select
        value={data.contributionMonthly.toFixed(3)}
        onChange={settingSelectHandler('contributionMonthly')}
      >
        {
          data.parameters.autoContributeChoices.map((choice) => (
            <option key={`choice-${choice}`} value={choice.toFixed(3)}>
              {getString('contributionUpTo')} {choice.toFixed(3)} BAT&nbsp;
              ({((choice * data.parameters.rate) || 0).toFixed(2)} USD)
            </option>
          ))
        }
      </select>
    )
  }

  function renderConfig () {
    return (
      <>
        <ConfigHeader />
        <PanelItem label={getString('contributionMonthly')}>
          {renderMonthlyAmountSelect()}
        </PanelItem>
        <PanelItem label={getString('contributionMinTime')}>
          <select
            onChange={settingSelectHandler('contributionMinTime')}
            value={data.contributionMinTime || 0}
          >
            <option value='5'>{getString('contributionTime5')}</option>
            <option value='8'>{getString('contributionTime8')}</option>
            <option value='60'>{getString('contributionTime60')}</option>
          </select>
        </PanelItem>
        <PanelItem label={getString('contributionMinVisits')}>
          <select
            onChange={settingSelectHandler('contributionMinVisits')}
            value={data.contributionMinVisits || 0}
          >
            <option value='1'>{getString('contributionVisit1')}</option>
            <option value='5'>{getString('contributionVisit5')}</option>
            <option value='10'>{getString('contributionVisit10')}</option>
          </select>
        </PanelItem>
        <PanelItem label={getString('contributionShowNonVerified')}>
          <ToggleButton
            checked={data.contributionNonVerified}
            onChange={settingToggleHandler('contributionNonVerified')}
          />
        </PanelItem>
        <PanelItem label={getString('contributionVideos')}>
          <ToggleButton
            checked={data.contributionVideos}
            onChange={settingToggleHandler('contributionVideos')}
          />
        </PanelItem>
      </>
    )
  }

  function renderDisabled () {
    return (
      <>
        <style.terms>
          {
            formatMessage(getString('tosAndPp'), {
              placeholders: {
                $1: getString('contributionTitle')
              },
              tags: {
                $2: (content) => (
                  <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                    {content}
                  </NewTabLink>
                ),
                $4: (content) => (
                  <NewTabLink key='privacy' href={urls.privacyPolicyURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </style.terms>
        <style.description>
          {getString('contributionDesc')}
        </style.description>
        <style.disabled>
          {getString('contributionDisabledText1')}&nbsp;
          {getString('contributionDisabledText2')}
        </style.disabled>
      </>
    )
  }

  function renderLimited () {
    return (
      <>
        <style.description>
          {getString('contributionDesc')}
        </style.description>
        {
          activityList.length > 0 &&
            <style.publisherSupport>
              <style.publisherCount>
                {activityList.length}
              </style.publisherCount>
              <div>{publisherCountText}</div>
            </style.publisherSupport>
        }
      </>
    )
  }

  function renderContent () {
    if (!data.enabledContribute) {
      return renderDisabled()
    }

    if (userType === 'unconnected') {
      return renderLimited()
    }

    return (
      <>
        <style.description>
          {getString('contributionDesc')}
        </style.description>
        <PanelItem label={getString('contributionMonthly')}>
          {renderMonthlyAmountSelect()}
        </PanelItem>
        <PanelItem label={getString('contributionNextDate')}>
          <MonthDay date={new Date(data.reconcileStamp * 1000)} />
        </PanelItem>
        <PanelItem label={getString('contributionSites')}>
          {activityList.length}
        </PanelItem>
        {renderTable(maxTableSize)}
        {
          canShowModal &&
            <style.showAll>
              <button onClick={toggleModal}>
                {getString('showAll')}
              </button>
            </style.showAll>
        }
      </>
    )
  }

  function renderModal () {
    if (!showModal) {
      return null
    }

    const getProfile = (
      publisher: Rewards.Publisher | Rewards.ExcludedPublisher
    ) => {
      const verified = publisher.status > 0
      const favIcon = publisher.favIcon && verified
        ? publisher.favIcon
        : publisher.url

      return {
        name: publisher.name,
        verified,
        provider: (publisher.provider ? publisher.provider : undefined),
        src: `chrome://favicon/size/64@1x/${favIcon}`
      }
    }

    const contributeRows = activityList.map((item) => ({
      profile: getProfile(item),
      url: item.url,
      attention: item.percentage,
      onRemove: () => { actions.excludePublisher(item.id) }
    }))

    const excludedRows = data.excludedList.map((item) => ({
      profile: getProfile(item),
      url: item.url,
      attention: 0,
      onRemove: () => { actions.restorePublisher(item.id) }
    }))

    return (
      <ModalContribute
        rows={contributeRows}
        onRestore={onRestore}
        excludedRows={excludedRows}
        activeTabId={modalTab}
        onTabChange={onTabChange}
        onClose={toggleModal}
      />
    )
  }

  // Don't show AC panel for users in bitflyer-enabled regions.
  if (data.externalWalletProviderList.includes('bitflyer')) {
    return null
  }

  return (
    <SettingsPanel>
      <style.root data-test-id='auto-contribute-panel'>
        {renderModal()}
        <PanelHeader
          title={getString('contributionTitle')}
          enabled={data.enabledContribute}
          showConfig={showConfig}
          onShowConfigChange={
            userType === 'unconnected' ? undefined : setShowConfig
          }
          onEnabledChange={onEnabledChange}
        />
        {showConfig ? renderConfig() : renderContent()}
      </style.root>
    </SettingsPanel>
  )
}
