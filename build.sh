if [ ! -d build ]
then
  echo "Creating build directory..."
  mkdir -p build
fi

cd build && cmake ..