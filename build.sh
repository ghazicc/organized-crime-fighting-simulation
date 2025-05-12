if [ ! -d build ]
then
  echo "Creating build directory..."
  mkdir -p build
fi

cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build \
      -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake \
      -DCONAN_COMMAND=conan -DBUILD_TESTING=ON