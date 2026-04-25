// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

export const BackArrow = <svg width="8" height="15" viewBox="0 0 8 15" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M2.41912 7.13281L7.18936 1.40853C7.46435 1.07854 7.41976 0.588097 7.08977 0.313104C6.75978 0.0381098 6.26934 0.082696 5.99435 0.412688L0.809178 6.63489C0.568816 6.92333 0.568816 7.3423 0.809178 7.63073L5.99435 13.8529C6.26934 14.1829 6.75978 14.2275 7.08977 13.9525C7.41976 13.6775 7.46435 13.1871 7.18936 12.8571L2.41912 7.13281Z" fill="currentColor" />
</svg>

export const ArrowRight = <svg width="16" height="15" viewBox="0 0 16 15" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M9.76613 7.85573L4.78172 2.18596C4.49438 1.85911 4.54097 1.37334 4.88577 1.10097C5.23058 0.828593 5.74304 0.872754 6.03038 1.1996L11.4484 7.36255C11.6995 7.64824 11.6995 8.06322 11.4484 8.34891L6.03038 14.5119C5.74304 14.8387 5.23058 14.8829 4.88577 14.6105C4.54097 14.3381 4.49438 13.8523 4.78172 13.5255L9.76613 7.85573Z" fill="currentColor" />
</svg>

export const channelIcons: { [category: string]: JSX.Element } = {
  'default': <Icon name='product-brave-news' />,
  'Brave': <Icon>
    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24"><path fill="#6B7084" fillRule="evenodd" d="m20.69 8.47-.628-1.675.436-.962a.324.324 0 0 0-.068-.366l-1.188-1.18a1.947 1.947 0 0 0-1.995-.454l-.332.113L15.178 2H8.794L7.079 3.97l-.323-.112a1.952 1.952 0 0 0-2.013.46l-1.21 1.2a.258.258 0 0 0-.054.292l.456.998-.625 1.674.405 1.51L5.56 16.87a3.503 3.503 0 0 0 1.382 1.963s2.24 1.549 4.45 2.956c.195.124.398.214.617.21.218.004.421-.086.615-.21 2.483-1.596 4.447-2.962 4.447-2.962a3.504 3.504 0 0 0 1.38-1.966l1.836-6.88.404-1.512Z" clipRule="evenodd" /><path fill="#fff" fillRule="evenodd" d="m18.728 8.801-.03.09-.045.16c-.122.15-.38.437-.573.638l-1.773 1.848c-.193.201-.302.453-.192.707l.24.578c.11.254.12.674.014.956-.107.287-.291.54-.533.734l-.185.148a.9.9 0 0 1-.86.101l-.816-.38a4.234 4.234 0 0 1-.843-.55l-.773-.681a.346.346 0 0 1-.02-.505l1.883-1.245c.233-.154.357-.44.224-.683l-.67-1.194c-.132-.243-.185-.567-.117-.719.068-.152.339-.356.602-.454l2.185-.796c.264-.097.25-.198-.03-.224l-1.397-.102c-.28-.026-.486.014-.758.088l-1.056.257c-.271.074-.329.357-.278.628l.436 2.317c.051.271.076.545.056.607-.02.063-.262.164-.537.225l-.361.08a2.837 2.837 0 0 1-1 .007l-.438-.091c-.276-.058-.518-.156-.538-.219-.02-.063.004-.336.055-.607l.433-2.318c.05-.271-.007-.554-.278-.628L9.698 7.32c-.272-.074-.478-.113-.758-.087l-1.396.103c-.28.026-.294.127-.03.224l2.185.794c.264.097.535.301.603.453.068.152.015.476-.117.72l-.668 1.193c-.132.244-.008.53.225.683l1.884 1.243a.346.346 0 0 1-.018.505l-.773.682a4.246 4.246 0 0 1-.842.552l-.816.381a.902.902 0 0 1-.86-.1l-.185-.148a1.727 1.727 0 0 1-.543-.756 1.444 1.444 0 0 1 .022-.933l.24-.578c.109-.255 0-.507-.194-.708L5.882 9.696c-.193-.2-.451-.487-.573-.636l-.047-.16-.028-.09c-.003-.104.034-.433.077-.522.043-.088.207-.348.365-.577l.38-.55c.158-.23.43-.593.606-.808l.557-.684c.175-.216.325-.392.348-.39 0-.002.228.04.504.092l.844.158.678.127c.097.018.395-.037.663-.122l.607-.192c.268-.086.674-.198.903-.25l.212.003.212-.003c.23.052.636.163.904.248l.607.192c.268.085.567.14.663.122a284 284 0 0 0 .56-.106l.118-.022.843-.16c.277-.052.504-.094.52-.093.008 0 .157.174.333.39l.558.683c.176.216.45.579.607.807l.38.55c.159.229.406.644.421.739a2.3 2.3 0 0 1 .024.36Zm-6.635 5.516c.025 0 .258.086.519.192l.241.098c.26.106.679.294.93.419l.713.354c.25.125.269.359.04.52l-.608.425c-.23.16-.59.44-.8.62l-.767.655a.601.601 0 0 1-.758.002c-.206-.178-.548-.47-.76-.65a12.42 12.42 0 0 0-.802-.614l-.606-.42c-.23-.158-.214-.393.036-.52l.716-.366c.25-.127.668-.318.928-.424l.241-.098a5.18 5.18 0 0 1 .518-.193h.22Z" clipRule="evenodd" /></svg>
  </Icon>,
  'Business': <Icon name='news-business' />,
  'Cars': <Icon name='news-car' />,
  'Crypto': <Icon name='crypto-wallets' />,
  'Culture': <Icon name='news-culture' />,
  'Entertainment': <Icon name='news-entertainment' />,
  'Entertainment News': <Icon name='news-entertainment' />,
  'Fashion': <Icon name='news-fashion' />,
  'Film and TV': <Icon name='news-filmandtv' />,
  'Food': <Icon name='news-food' />,
  'Fun': <Icon name='news-fun' />,
  'Gaming': <Icon name='news-gaming' />,
  'Health': <Icon name='news-health' />,
  'Home': <Icon name='news-home' />,
  'Music': <Icon name='news-music' />,
  'Politics': <Icon name='news-politics' />,
  'Regional News': <Icon name='news-regional' />,
  'Science': <Icon name='news-science' />,
  'Sports': <Icon name='news-sports' />,
  'Travel': <Icon name='news-travel' />,
  'Technology': <Icon name='news-technology' />,
  'Tech News': <Icon name='news-technology' />,
  'Tech Reviews': <Icon name='news-technology' />,
  'Top News': <Icon name='news-topnews' />,
  'US News': <Icon name='news-regional' />,
  'Weather': <Icon name='news-weather' />,
  'World News': <Icon name='news-worldnews' />
}
