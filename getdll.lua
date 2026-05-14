local lfs = require('lfs')
local dllpath = os.getenv('CPATH') .. '\\..\\dll64\\'


function table.find (t, x)
for k, v in pairs(t) do
if v==x then return k end
end end

local function addToDLLs (dll, t)
if table.find(t, dll) then return end
local fr <close> = io.open(dllpath..dll, 'rb')
if not fr then return end
table.insert(t, dll)
end

local function dllList (fn, t)
t = t or {}
local dfp <close> = io.popen('objdump -p ' .. fn .. ' | grep -i "DLL Name:"', 'r')
for l in dfp:lines() do
local dll = l:sub(12)
addToDLLs(dll, t)
end end

local dlls = {}
for i = 1, #arg do
local dll = arg[i]
if dll:lower():match('%.dll$') then addToDLLs(dll, dlls)
else dllList(dll, dlls)
end end

do local i = 1 repeat
local dll = dlls[i]
dllList(dllpath..dll, dlls)
i = i+1
until i > #dlls end

for i, dll in ipairs(dlls) do
print(string.format('Check %s', dll))
local curmd = lfs.attributes(dll, 'modification') or 0
local defmd = lfs.attributes(dllpath..dll, 'modification') or 0
local cursize = lfs.attributes(dll, 'size') or 0
local defsize = lfs.attributes(dllpath..dll, 'size') or 0
if curmd==defmd or cursize==defsize then goto continue end
local src = curmd>defmd and dll or dllpath..dll
local dst = curmd<defmd and dll or dllpath..dll
local fr <close> = io.open(src, 'rb')
if not fr then goto continue end
local fw <close> = io.open(dst, 'wb')
if not fw then goto continue end
fw:write(fr:read('a'))
print(string.format('Copied %s to %s', src, dst))
::continue:: end
