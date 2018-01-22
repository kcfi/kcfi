-- kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
--
-- This software was developed by Joao Moreira <jmoreira@suse.de>
-- at Universidade Estadual de Campinas, SP, Brazil, in 2015.
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

function addEntry(f, id)
  os.execute("touch secondary.cfi")
  cfif = io.open("./secondary.cfi", "r")
  if not cfif then
    print("Problem opening secondary.cfi")
    os.exit()
  end

  new = io.open("./secondary.new", "w")
  if not new then
    print("Problem opening secondary.new")
    os.exit()
  end

  added = false
  newline = false

  for l in cfif:lines() do
    a = string.match(l, "^" .. f .. ",")
    if a then
      b = string.match(l, id)
      if not b then
        newline = l .. "," .. id
      else
        newline = l
      end
    end
    if newline then
      new:write(newline .. "\n")
      added = true
      newline = false
    else
      new:write(l .. "\n")
    end
  end
  if not added then
    new:write(f .. "," .. id .. "\n")
  end
  cfif:close()
  new:close()
  os.execute("mv ./secondary.new ./secondary.cfi")
end

function parse_syscalls_32()
  os.execute("cat ./arch/x86/syscalls/syscall_32.tbl | grep -v \\# | sed -e's/[\\t]\\+/\\t/g' | cut -f4,5 | sed -e's/[\\t]\\+/\\n/g' > secondary32.tmp")

  sec = io.open("./secondary32.tmp", "r")
  if not sec then
    print("Problem opening secondary32.tmp")
    os.exit()
  end

  syscalls = {}
  i = 1
  for line in sec:lines() do
    syscall = string.match(line, "%s*([%w_]+)")
    if syscall then
      syscalls[i] = syscall
      i = i + 1
    end
  end
  sec:close()
  return syscalls
end

function parse_syscalls_64()
  os.execute("cat ./arch/x86/syscalls/syscall_64.tbl | grep -v \\# | sed -e's/[\\t]\\+/\\t/g' | cut -f4 > secondary64.tmp")

  sec = io.open("./secondary64.tmp", "r")
  if not sec then
    print("Problem opening secondary64.tmp")
    os.exit()
  end

  syscalls = {}
  i = 1
  for line in sec:lines() do
    syscall = string.match(line, "%s*([%w_]+)")
    if syscall then
      syscalls[i] = syscall
      i = i + 1
    end
  end
  sec:close()
  return syscalls
end

function parse_ptregs_32()
  os.execute("cat ./arch/x86/ia32/ia32entry.S | grep PTREGSCALL | grep -v macro > secondaryptregs.tmp")

  sec = io.open("./secondaryptregs.tmp", "r")
  if not sec then
    print("Problem opening secondaryptregs.tmp")
    os.exit()
  end

  syscalls = {}
  i = 1
  for line in sec:lines() do
    syscall = string.match(line, "%s*.*%s.*,%s([%w_]+)")
    if syscall then
      syscalls[i] = syscall
      i = i + 1
    end
  end
  sec:close()
  return syscalls
end

function unfold_aliases(fs)
  fs_unfolded = {}
  i = 1
  while i < #fs do
    fs_unfolded[#fs_unfolded+1] = fs[i]
    os.execute("cat cfi.dump | grep Alias | grep \": " .. fs[i] .. " Aliasee:\" | cut -d: -f3 > alias.tmp")
    alias = io.open("./alias.tmp", "r")
    if not alias then
      print("Problem opening alias.tmp")
      os.exit()
    end
    c = 0
    for line in alias:lines() do
      a = string.match(line, "%s*([%w_]+)")
      if a then
        fs_unfolded[#fs_unfolded+1] = a
        c = c + 1
      end
    end
    alias:close()
    i = i + 1
  end
  return fs_unfolded
end

os.execute("touch ./secondary.cfi")
os.execute("rm ./secondary.cfi")

syscalls64 = parse_syscalls_64()
syscalls64 = unfold_aliases(syscalls64)
i = 1
while i <= #syscalls64 do
  addEntry(syscalls64[i], "00deadca")
  i = i + 1
end

syscalls32 = parse_syscalls_32()
syscalls32 = unfold_aliases(syscalls32)
i = 1
while i <= #syscalls32 do
  addEntry(syscalls32[i], "00deadcb")
  i = i + 1
end

syscallspt = parse_ptregs_32()
syscallspt = unfold_aliases(syscallspt)
i = 1
while i <= #syscallspt do
  addEntry(syscallspt[i], "00deadcb")
  i = i + 1
end

addEntry("kernel_init","00dead01")
addEntry("____call_usermodehelper","00dead01")
addEntry("copy_user_handle_tail","00dead02")
