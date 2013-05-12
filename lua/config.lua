
-- this script to store the basic configuration for game program itself
-- and it is a little different from config.ini

config = {
	version = "20120405",
	version_name = "东海版",
	mod_name = "official",
	kingdoms = {"pirate", "government", "citizen", "noble"},
	package_names = {
	"StandardCard",
        "Disaster",

        "Standard",
        "Alabastan",
        --"Skypiea",
        "Water7",
        "ThrillerBark",
        "Test",
        --"Sgs","Wind","God", --三国杀，用于测试和参考
	},

	scene_names = {
	--"Guandu",
        --"Fancheng",
        --"Couple",
        --"Zombie",
        --"Impasse",
        --"Custom",
	},

        --color_wei = "#547998",
        --color_shu = "#D0796C",
        --color_wu = "#4DB873",
        --color_qun = "#8A807A",
        --color_god = "#96943D",

        color_pirate = "#547998",
        color_citizen = "#4DB873",
        color_government = "#8A807A",
        color_noble = "#96943D",
}

for i=1, 21 do
	local scene_name = ("MiniScene_%02d"):format(i)
	table.insert(config.scene_names, scene_name)
end

