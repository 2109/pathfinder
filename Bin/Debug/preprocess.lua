require "lfs"
local nav = require "nav.core"

local source_file,output_path = ...
print(string.format("source file:%s",source_file))
print(string.format("output path:%s",output_path))

local function distance3(Point1, Point2)
	return math.sqrt( (Point1[1]-Point2[1])*(Point1[1]-Point2[1]) + (Point1[2]-Point2[2])*(Point1[2]-Point2[2]) + (Point1[3]-Point2[3])*(Point1[3]-Point2[3]) )
end

local function get_suffix(filename)
    return filename:match(".+%.(%w+)$")
end

local function list_dir(path,recursive,suffix,is_path_name,r_table)
    r_table = r_table or {}

    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            local f = path..'/'..file

            local attr = lfs.attributes (f)
            if attr.mode == "directory" and recursive then
                list_dir(f, recursive, suffix, is_path_name,r_table)
            else
                local target = file
                if is_path_name then target = f end

                if suffix == nil or suffix == "" or suffix == get_suffix(f) then
                    table.insert(r_table, target)
                end
            end
        end
    end

    return r_table
end

local function preprocess(file,output)
	local data = nav.read_nav(file)

	local name = file:match(".+%/(%d+).nav")

	--预处理重复的点
	local vertex_set = {}
	local vertex_map = {}
	for i,vertex in ipairs(data.v) do
		local flag = false
		for tmp_i,tmp in ipairs(vertex_set) do
			if distance3(tmp,vertex) < 1 then
				vertex_map[i] = tmp_i
				flag = true
				break
			end
		end
		if not flag then
			table.insert(vertex_set,vertex)
			vertex_map[i] = #vertex_set
		end
	end

	--替换重复的点
	for _,info in ipairs(data.p) do
		for i,index in ipairs(info) do
			info[i] = vertex_map[index+1] - 1
		end
	end

	data.v = vertex_set

	if #vertex_set == 0 then
		os.exit(1)
	end

	if #data.p == 0 then
		os.exit(1)
	end

	nav.write_nav(string.format("%s/%s.nav",output,name),data)

	print(string.format("%s======>%s/%s.nav",file,output,name))

	local ctx = nav.create(string.format("%s/%s.nav",output,name))
	ctx:create_tile(400)
	nav.write_tile(string.format("%s/%s.nav.tile",output,name),400,ctx:tile_info())
	print(string.format("%s======>%s/%s.nav.tile",file,output,name))
end


preprocess(source_file,output_path)

