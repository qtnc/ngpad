function sortedpairs (t)
local keys = {}
for key in pairs(t) do table.insert(keys, key) end
table.sort(keys)
return coroutine.wrap(function()
for index, key in ipairs(keys) do
coroutine.yield(key, t[key])
end end) end

function string:split (sep)
local previous = 1
local t = {}
repeat
local start, finish = self:find(sep, previous)
if not start then break end
local s = self:sub(previous, start -1)
if #s>0 then table.insert(t, s) end
previous = finish +1
until not finish
local s = self:sub(previous)
if #s>0 then table.insert(t, s) end
return t
end

function mkgsprop (val, name, readable, writable)
local pval = val:split('%s*:%s*')
local ptype = table.remove(pval, 1)
local pdesc = table.concat(pval, ': ')
return { name = name, type = ptype, readable = readable, writable = writable, description = pdesc }
end

function mkmparam (val) 
local pval = val:split('%s*:%s*')
local pname = table.remove(pval, 1)
local ptype = table.remove(pval, 1)
local pdef = table.remove(pval, 1)
local pdesc = table.concat(pval, ': ')
return { type=ptype, name=pname, defaultValue=pdef, description=pdesc }
end

function mkmparams (method, seq)
for i = 2, #seq do
local tag, val  = table.unpack(seq[i])
if tag=='P' then
table.insert(method.parameters, mkmparam(val))
elseif tag=='R' then
local rval = val:split('%s*:%s*')
local rtype = table.remove(rval, 1)
local rdesc = table.concat(rval, ': ')
table.insert(method.returns, { type = rtype, description = rdesc })
end end end

function writemethod (write, tName, mName, method)
local pNames = {}
for i, param in ipairs(method.parameters) do

local pName = param.name
if param.defaultValue and param.defaultValue~='' and param.defaultValue~='nil' and param.defaultValue~="''" and param.defaultValue~='""' then
pName = param.name .. '=' .. param.defaultValue
param.hasDefaultValue = true
end
table.insert(pNames, pName)
end

local mSep = '.'
if method.self then
mSep = ':'
tName = tName:sub(1, 1):lower() .. tName:sub(2)
end
local fName = tName and #tName>0 and tName..mSep..mName or mName
local pNames = table.concat(pNames, ', ')
write(string.format('### %s (%s)', fName, pNames))
write(method.description)
write()

if method.parameters and #method.parameters>0 then
write('**Parameters:**')
for i, param in ipairs(method.parameters) do
write(string.format('* %s: %s: %s', param.name, param.type, param.description))
end
write()
end -- parameters

if method.returns and #method.returns>0 then
write('**Returns:**')
for i, ret in ipairs(method.returns) do
write(string.format('* %s: %s', ret.type, ret.description))
end
write()
end
end -- write method

local outFile = table.remove(arg)
local allTypes = {}
local curType

local output = {}
local write = function(s)
table.insert(output, s or '')
end

for index, filename in ipairs(arg) do
local seq = {}
local previousMatched = false
for line in io.lines(filename) do
local c, s = line:match('^%s*//(%u)%s(.-)$')
if previousMatched then
previousMatched=false
local name = line:match('%s*%u+%s*%(%s*([%w%d_]+)') or line:match('"([%w%d]+)"')
if name and not c then
local tag, val = table.unpack(seq[1])
if tag=='T' then
curType = allTypes[name] or { methods = {}, properties = {}, constructors = {}, constants={}, name = name, description = val }
allTypes[name] = curType
elseif tag=='M' then
local method = { parameters = {}, returns = {}, self=true, description = val }
mkmparams(method, seq)
curType.methods[name] = method
elseif tag=='F' then
local method = { parameters = {}, returns={}, self=false, description = val }
mkmparams(method, seq)
curType.methods[name] = method
elseif tag=='G' then
curType.properties[name] = mkgsprop(val, name, true, false)
elseif tag=='A' then
curType.properties[name] = mkgsprop(val, name, true, true)
elseif tag=='S' then
curType.properties[name] = mkgsprop(val, name, false, true)
elseif tag=='K' then
curType.constants[name] = mkgsprop(val, name, true, false)
elseif tag=='C' then
local ctor = { parameters = {}, description = val }
mkmparams(ctor, seq)
table.insert(curType.constructors, ctor)
end --  switch tag
seq = {}
end -- name match
end--previousMatched
if c then 
table.insert(seq, {c,s}) 
previousMatched=true
else
seq = {}
end -- comment match
end -- lines
end -- files

for tpName, tp in sortedpairs(allTypes) do 
write('## '.. tpName)
write(tp.description)
write()

if tp.constructors and #tp.constructors then
for i, ctor in ipairs(tp.constructors) do
writemethod(write, nil, tpName, ctor)
end end -- constructors

if tp.methods and next(tp.methods) then
for name, method in sortedpairs(tp.methods) do
writemethod(write, tpName, name, method)
end  end -- methods

if tp.properties and next(tp.properties) then
write[[
### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----]]
for name, prop in sortedpairs(tp.properties) do
write(string.format('%s | %s | %s | %s | %s', prop.name, prop.type, prop.readable, prop.writable, prop.description))
end
write()
end -- properties

if tp.constants and next(tp.constants) then
write[[
### Constants
Name | Type | Description
-----|-----|-----]]
for name, prop in sortedpairs(tp.constants) do
write(string.format('%s | %s | %s', prop.name, prop.type, prop.description))
end
write()
end -- constants

end -- types

local inFp = io.open(outFile..'g', 'r')
local content = inFp:read('a')
inFp:close()

local start, finish = content:find('@@generated')
content = content:sub(1, start -1) .. table.concat(output, '\n') .. content:sub(finish+1)

local outFp = io.open(outFile, 'w')
outFp:write(content)
outFp:close()
