-- translation for StandardPackage

local t = {
	["standard_cards"] = "标准卡牌包",

	["slash"] = "杀",
	[":slash"] = "基本牌\
出牌时机：出牌阶段\
使用目标：攻击范围内的一名其他角色\
作用效果：【杀】对目标角色造成1点伤害",
	["slash-jink"] = "%src 使用了【杀】，请打出一张【闪】",

	["jink"] = "闪",
	[":jink"] = "基本牌\
出牌时机：以你为目标的【杀】开始结算时\
使用目标：以你为目标的【杀】\
作用效果：抵消目标【杀】的效果",
	["#Slash"] = "%from 对 %to 使用了【杀】",
	["#Jink"] = "%from 使用了【闪】",

	["wine"] = "酒",
	[":wine"] = "基本牌\
出牌时机：1、出牌阶段；2、有角色处于濒死状态时\
使用目标：1、你；2、处于濒死状态的一名角色\
作用效果：目标角色回复1点体力",

	["hammer"] = "5吨重锤",
	[":hammer"] = "装备牌·武器\
攻击范围：１\
武器特效：出牌阶段，你可以使用任意张【杀】",

	["okama_microphone"] = "人妖话筒",
	[":okama_microphone"] = "装备牌·武器\
攻击范围：２\
武器特效：当你使用【杀】指定一名异性角色为目标后，你可以令其选择一项：弃置一张手牌，或令你摸一张牌",
	["okama-microphone-card"] = "%src 发动了雌雄双股剑特效，您必须弃置一张手牌或让 %src 摸一张牌",
	["okama_microphone:yes"] = "您可以让对方选择自弃置一牌或让您摸一张牌",

	["wado_ichimonji"] = "和道一文字",
	[":wado_ichimonji"] = "装备牌·武器\
攻击范围：２\
武器特效：<b>锁定技</b>，当你使用【杀】指定一名角色为目标后，无视其防具",

	["sandai_kitetsu"] = "三代鬼彻",
	[":sandai_kitetsu"] = "装备牌·武器\
攻击范围：３\
武器特效：当你使用的【杀】被【闪】抵消时，你可以立即对相同的目标再使用一张【杀】直到【杀】生效或你不愿意出了为止",
	["sandai_kitetsu-slash"] = "您可以再打出一张【杀】发动鬼彻的追杀效果",

	["shigure"] = "时雨",
	[":shigure"] = "装备牌·武器\
攻击范围：３\
武器特效：你可以将两张手牌当【杀】使用或打出",

	["impact_dial"] = "冲击贝",
	[":impact_dial"] = "装备牌·武器\
攻击范围：３\
武器特效：<b>锁定技</b>，当你使用的【杀】被【闪】抵消时，你弃置两张牌或流失1点体力，然后此【杀】依然造成伤害",
	["@impact_dial"] = "你可再弃置两张牌（包括装备）或流失1点体力使此杀强制命中",
	["#ImpactDialSkill"] = "%from 使用了【%arg】的技能，对 %to 强制命中",

	["yubashiri"] = "雪走",
	[":yubashiri"] = "装备牌·武器\
攻击范围：４\
武器特效：当你使用【杀】时，且此【杀】是你最后的手牌，你可以额外指定至多两个目标",

	["kabuto"] = "独角仙",
	[":kabuto"] = "装备牌·武器\
攻击范围：５\
武器特效：当你使用【杀】对目标角色造成伤害时，你可以弃置其装备区里的一张坐骑牌",
	["kabuto:yes"] = "弃置对方的一匹马",
	["kabuto:dhorse"] = "防御坐骑(+1)",
	["kabuto:ohorse"] = "进攻坐骑(-1)",

	["flame_dial"] = "炎贝",
	[":flame_dial"] = "装备牌·武器\
攻击范围：４\
武器特效：你可以将你的任一普通杀当作具火焰伤害的杀来使用",
	["flame_dial:yes"] = "你可将此普通【杀】当具火焰伤害的【杀】使用或打出",

	["shusui"] = "秋水",
	[":shusui"] = "装备牌·武器\
攻击范围：２\
武器特效：<b>锁定技</b>，当你使用【杀】对目标角色造成伤害时，若其没有手牌，此伤害+1",
	["#ShusuiEffect"] = "%from 装备的秋水生效，无手牌对象 %to 的伤害从 %arg 增加到 %arg2",

	["milky_dial"] = "云贝",
	[":milky_dial"] = "装备牌·防具\
防具效果：每当你需要使用（或打出）一张【闪】时，你可以进行一次判定：若结果为红色，则视为你使用（或打出）了一张【闪】；若为黑色，则你仍可从手牌里使用（或打出）。\
★由八卦使用或打出的【闪】，并非从你的手牌中使用或打出",
	["milky_dial:yes"] = "进行一次判定，若判定结果为红色，则视为你打出了一张【闪】",

	["soul_solid"] = "魂之丧剑",
	[":soul_solid"] = "装备牌·武器\
攻击范围：２\
武器特效：当你使用【杀】对目标角色造成伤害时，若该角色有牌，你可以防止此伤害，改为依次弃置其两张牌（弃完第一张再弃第二张）",
	["soul_solid:yes"] = "您可以弃置其两张牌",

	["cloak"] = "披风",
	[":cloak"] = "装备牌·防具\
防具效果：<b>锁定技</b>，黑色的【杀】对你无效",

	["horse"] = "坐骑",
	
	[":+1 horse"] = "装备牌·坐骑\
坐骑效果：其他角色计算与你的距离时，始终+1（你可以理解为一种防御上的优势）不同名称的+1坐骑，其效果是相同的",
	["waver"] = "威霸",

	[":-1 horse"] = "装备牌·坐骑\
坐骑效果：你计算与其他角色的距离时，始终-1（你可以理解为一种进攻上的优势）不同名称的-1坐骑，其效果是相同的",
	["mini_merry"] = "迷你梅利Ⅱ",

	["amazing_grace"] = "黄金乡",
	[":amazing_grace"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：所有角色。\
作用效果：你从牌堆亮出等同于现存角色数量的牌，然后按行动顺序结算，每名目标角色选择并获得其中的一张",

	["all_blue"] = "ALL BLUE",
	[":all_blue"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：所有角色。\
作用效果：按行动顺序结算，目标角色回复1点体力",

	["neptunian_attack"] = "海王类",
	[":neptunian_attack"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：除你以外的所有角色。\
作用效果：按行动顺序结算，除非目标角色打出1张【杀】，否则该角色受到你对其造成的1点伤害",
	["neptunian-attack-slash"] = "%src 使用了【海王类】，请打出一张【杀】来响应",

	["buster_call"] = "屠魔令",
	[":buster_call"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：除你以外的所有角色。\
作用效果：按行动顺序结算，除非目标角色打出1张【闪】，否则该角色受到你对其造成的1点伤害",
	["buster-call-jink"] = "%src 使用了【屠魔令】，请打出一张【闪】以响应",

	["busou_haki"] = "霸气·武装色",
	[":busou_haki"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：你\
作用效果：令自己的下一张使用的【杀】造成的伤害+1（每阶段限使用1次）。",
	["#Drank"] = "%from 使用了【霸气（武装色）】，下一张【杀】将具有伤害 +1 的效果",
	["#UnsetDrank"] = "%from 杀的结算完毕，【霸气】的效果消失",
	["#UnsetDrankEndOfTurn"] = "%from 的回合结束，【霸气】的效果消失",
	["#BusouHakiBuff"] = "%from 使用了【%arg】，对 %to 造成的杀伤害 +1",

	["nullification"] = "霸气·见闻色",
	[":nullification"] = "效果牌\
出牌时机：目标效果对目标角色生效前。\
使用目标：目标效果。\
作用效果：抵消目标效果牌对一名角色产生的效果，或抵消另一张【见闻色霸气】产生的效果",

	["collateral"] = "线人偶",
	[":collateral"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：装备区里有武器牌的一名其他角色A。（你需要在此阶段选择另一个A使用【杀】能攻击到的角色B）。\
作用效果：A需对B使用一张【杀】，否则A必须将其装备区的武器牌交给你",
	["collateral-slash"] = "%src 使用了【线人偶】，目标是 %dest，请打出一张【杀】以响应",

	["duel"] = "决斗",
	[":duel"] = "效果牌\
出牌时机：出牌阶段\
使用目标：一名其他角色\
作用效果：由该角色开始，你与其轮流打出一张【杀】，首先不出【杀】的一方受到另一方造成的1点伤害",
	["duel-slash"] = "%src 向您决斗，您需要打出一张【杀】",

	["treasure_chest"] = "藏宝箱",
	[":treasure_chest"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：你。\
作用效果：摸两张牌",

	["snatch"] = "盗窃",
	[":snatch"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：与你距离1以内的一名其他角色。\
作用效果：你选择并获得其手牌区、装备区或判定区里的一张牌",

	["dismantlement"] = "拆解",
	[":dismantlement"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：一名其他角色。\
作用效果：你选择并弃置其手牌区、装备区或判定区里的一张牌",

	["negative_soul"] = "消极之魂",
	[":negative_soul"] = "延时效果牌\
出牌时机：出牌阶段。\
使用目标：一名其他角色。\
作用效果：将【消极之魂】放置于目标角色判定区里，目标角色回合判定阶段进行判定；若判定结果不为红桃，则跳过其的出牌阶段，将【消极之魂】置入弃牌堆",

	["lightning"] = "雷天候",
	[":lightning"] = "延时效果牌\
出牌时机：出牌阶段。\
使用目标：你。\
作用效果：将【雷天候】放置于你判定区里，目标角色回合判定阶段，进行判定；若判定结果为黑桃2-9之间（包括黑桃2和9），则将【雷天候】置入弃牌堆，【雷天候】对目标角色造成3点雷电伤害。若判定结果不为黑桃2-9之间（包括黑桃2和9），将【雷天候】移动到当前目标角色下家（【雷天候】的目标变为该角色）的判定区",

	["rain"] = "雨天候",
	[":rain"] = "延时效果牌\
出牌时机：出牌阶段。\
使用目标：你。\
作用效果：将【雨天候】放置于你判定区里，目标角色回合判定阶段，进行判定；若判定结果为红桃2-9之间（包括红桃2和9），则将【雨天候】置入弃牌堆，目标角色的红桃牌均视为黑桃牌直到回合结束。若判定结果不为黑桃2-9之间（包括黑桃2和9），将【雨天候】移动到当前目标角色下家（【雨天候】的目标变为该角色）的判定区",
	["raineffect"] = "雨天候",
	["#RainEffectRetrialResult"] = "受到【雨天候】影响，判定牌由 %",

	["tornado"] = "风天候",
	[":tornado"] = "延时效果牌\
出牌时机：出牌阶段。\
使用目标：你。\
作用效果：将【风天候】放置于你判定区里，目标角色回合判定阶段，进行判定；若判定结果为方片2-9之间（包括方片2和9），则将【风天候】置入弃牌堆，目标角色弃置其装备区内所有牌。若判定结果不为梅花2-9之间（包括梅花2和9），将【风天候】移动到当前目标角色下家（【风天候】的目标变为该角色）的判定区",

	["fire_slash"] = "火杀",
	[":fire_slash"] = "基本牌\
出牌时机：出牌阶段\
使用目标：攻击范围内的一名其他角色\
作用效果：【火杀】对目标角色造成1点火焰伤害",

	["thunder_slash"] = "雷杀",
	[":thunder_slash"] = "基本牌\
出牌时机：出牌阶段\
使用目标：攻击范围内的一名其他角色\
作用效果：【雷杀】对目标角色造成1点雷电伤害",

	["candle_wall"] = "蜡盾",
	[":candle_wall"] = "装备牌·防具\
防具效果：<b>锁定技</b>，【海王类】、【万箭齐发】和普通【杀】对你无效。当你受到火焰伤害时，此伤害+1",
	["#CandleWallDamage"] = "%from 装备【蜡盾】的效果被触发，由 %arg 点火焰伤害增加到 %arg2 点火焰伤害",

	["silver_lion"] = "白银狮子",
	[":silver_lion"] = "装备牌·防具\
防具效果：<b>锁定技</b>，当你受到伤害时，若该伤害多于1点，则防止多余的伤害；当你失去装备区里的【白银狮子】时，你回复1点体力",
	["#SilverLion"] = "%from 的防具【%arg2】防止了 %arg 点伤害，减至1点",

	["fire_attack"] = "火攻",
	[":fire_attack"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：一名有手牌的角色。\
作用效果：目标角色展示一张手牌，然后若你弃置一张与所展示牌相同花色的手牌，则【火攻】对其造成1点火焰伤害",
	["fire-attack-card"] = "您可以弃置一张与%dest所展示卡牌相同花色(%arg)的牌对%dest产生一点火焰伤害",
	["@fire-attack"] = "%src 展示的牌的花色为 %arg，请弃置与其相同花色的牌",

	["tama_dragon"] = "惊奇球髻",
	[":tama_dragon"] = "效果牌\
出牌时机：出牌阶段。\
使用目标：1、一至两名角色；2、不指定目标。\
作用效果：1、分别横置或重置目标武将牌，处于“连环状态”；2、将此牌置入弃牌堆，然后摸一张牌",

	["supply_shortage"] = "无空世界",
	[":supply_shortage"] = "延时效果牌\
出牌时机：出牌阶段。\
使用目标：距离为1的一名其他角色。\
作用效果：将【无空世界】放置于该角色的判定区里，目标角色回合判定阶段进行判定：若判定结果不为梅花，则跳过其摸牌阶段，将【无空世界】置入弃牌堆",
}

local ohorses = {"mini_merry"}
local dhorses = {"waver"}

for _, horse in ipairs(ohorses) do
	t[":" .. horse] = t[":-1 horse"]
end

for _, horse in ipairs(dhorses) do
	t[":" .. horse] = t[":+1 horse"]
end

return t