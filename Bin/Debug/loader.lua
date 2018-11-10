local nav = require "nav.core"

local file = "1001.nav"

local ctx = nav.create(string.format("./data/%s",file))
ctx:load_tile(string.format("./data/%s.tile",file))

print(ctx:raycast(27810,10949,27610,10939))
-- print(ctx:random_point(20956,36472))