set can-use-hw-watchpoints 0
define asst2
dir ~/Documents/comp3231/assignments/asst2-src/kern/compile/ASST2
target remote unix:.sockets/gdb
b panic
end
