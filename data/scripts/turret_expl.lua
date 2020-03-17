local C = {}

function C:init(unit)
  self.unit = unit
end

function C:onUpdate()
  if self.unit.age > 10 then
    self.unit.deleteMeNow = true
  end
end

return function(unit)
  local o = {}
  setmetatable(o, C)
  C.__index = C
  o:init(unit)
  return o
end