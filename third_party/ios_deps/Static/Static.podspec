Pod::Spec.new do |spec|
  spec.name = 'Static'
  spec.version = '4.0.0'
  spec.summary = 'Simple static table views for iOS in Swift.'
  spec.description = 'Static provides simple static table views for iOS in Swift.'
  spec.homepage = 'https://github.com/venmo/static'
  spec.license = { type: 'MIT', file: 'LICENSE' }
  spec.source = { git: 'https://github.com/venmo/Static.git', tag: "v#{spec.version}" }
  spec.author = { 'Venmo' => 'ios@venmo.com', 'Sam Soffes' => 'sam@soff.es' }

  spec.platform = :ios, '8.0'
  spec.frameworks = 'UIKit'
  spec.source_files = 'Static/*.{swift,h}'
end
