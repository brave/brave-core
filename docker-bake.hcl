
# build by running 
#   docker buildx bake brave-dev-${os}-${platform}
# or for everything: 
#  docker buildx bake brave-dev

# To accelerate buildkit you can connect it to RBE:

# k8s:
# docker buildx create \
#  --bootstrap \
#  --name=kube \
#  --driver=kubernetes \
#  --driver-opt=namespace=buildkit,replicas=4
# docker buildx bake --builder=kube brave-dev

# any remote node:
# docker buildx create \
#  --name remote \
#  --driver remote \
#  tcp://ec2-instance.aws.com 


group "default" {
  targets = ["brave-dev"]
}

target "brave-dev" {
  name       = "brave-dev-${os}-${arch}"
  target     = "brave-dev"
  dockerfile = "Dockerfile"
  

  args = {
    TARGET_OS   = os,
    TARGET_ARCH = arch
  }

  # TODO: remove invalid combinations
  # potentially list out all pairs explicitly
  matrix = {
    os = ["linux", "windows", "macos", "android", "ios"],
    arch = ["x64", "arm64"]
  }
}

target "brave-dev-prebuilt" {
  name       = "brave-dev-prebuilt-${buildType}-${os}-${arch}"
  target     = "brave-dev-prebuilt"
  dockerfile = "Dockerfile"
  
  args = {
    BUILD_TYPE = buildType,
    TARGET_OS   = os,
    TARGET_ARCH = arch
  }

  # TODO: remove invalid combinations
  # potentially list out all pairs explicitly
  matrix = {
    buildType = ["Static", "Component"],
    os = ["linux", "windows", "macos", "android", "ios"],
    arch = ["x64", "arm64"]
  }
}