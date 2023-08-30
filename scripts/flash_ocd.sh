TARGET=pico.elf

bash scripts/build.sh

echo "Goign to flash $TARGET using openocd"

sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "program build/$TARGET verify reset exit"
