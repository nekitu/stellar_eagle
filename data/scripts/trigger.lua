local C = {}

function C:init(unit)
  self.unit = unit
end

function C:onUpdate()
end

function C:onAppeared()
  game:fadeScreen(Color(0, 0, 0, 1), Color(1,0,0,1), 3, true)
end

return newInstance(C)