set can-use-hw-watchpoints 0
define asst2
dir ~/cs3231/asst2-src/kern/compile/ASST2
target remote unix:.sockets/gdb
b panic
end
