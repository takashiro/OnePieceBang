
--Rubber Pistol
sgs.ai_skill_invoke.rubberpistol = function(self, data)
	return true
end

local rubberpistol_skill={}
rubberpistol_skill.name="rubberpistol"
table.insert(sgs.ai_skills, rubberpistol_skill)
rubberpistol_skill.getTurnUseCard=function(self)
	if not self:slashIsAvailable() then return end
	return sgs.Card_Parse("@RubberPistolCard=.")
end

sgs.ai_skill_use_func.RubberPistolCard = function(card, use, self)
	local slash = sgs.Bang:cloneCard("slash", sgs.Card_NoSuit, 0)

	self:sort(self.enemies, "defense")
	for _, target in ipairs(self.enemies) do
		if self:isEnemy(target) and not self:slashProhibit(slash ,target) and self:slashIsEffective(slash,target) and self.player:inMyAttackRange(target) then
			use.card = sgs.Card_Parse("@RubberPistolCard=.")
			if use.to then
				use.to:append(target)
			end
			return
		end
	end
end

--Frety Wind
sgs.ai_skill_invoke.fretywind = function(self, data)
	local choice = sgs.ai_skill_choice.fretywind
	if(choice ~= "nothing") then
		return true
	end
	return false	 
end

sgs.ai_skill_choice.fretywind = function(self, choices)
	self:sort(self.enemies, "defense")
	local n1 = self:getCardsNum("Slash")
	local slash = sgs.Card_Parse(("slash[%s:%s]"):format(sgs.Card_NoSuit, 0))
	for _, enemy in ipairs(self.enemies) do
		local n2 = self:getCardsNum("Slash", enemy)
		if n1 >= n2 then
			return "duel"
		elseif not self:slashProhibit(slash ,enemy) then
			return "slash"
		end
	end
	return "nothing"
end

sgs.ai_skill_playerchosen.fretywind = sgs.ai_skill_playerchosen.zero_card_as_slash

--Clima
local clima_skill = {}
clima_skill.name = "clima"
table.insert(sgs.ai_skills, clima_skill)
clima_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	
	local clima_card
	
	self:sortByUseValue(cards, true)

	for _,card in ipairs(cards)  do
		if not card:inherits("TrickCard") and card:getSuit() ~= sgs.Card_Club and (inclusive or self:getOverflow() > 0) then
			local shouldUse = true

			if card:inherits("Armor") then
				if not self.player:getArmor() then shouldUse = false 
				elseif self:hasEquip(card) and not (card:inherits("SilverLion") and self.player:isWounded()) then shouldUse = false
				end
			end

			if card:inherits("Weapon") then
				if not self.player:getWeapon() then shouldUse = false
				end
			end
			
			if card:inherits("Slash") then
				local dummy_use = {isDummy = true}
				if self:getCardsNum("Slash") == 1 then
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
			end

			if shouldUse then
				clima_card = card
				break
			end
		end
	end

	if clima_card then
		local suit = clima_card:getSuitString()
		local number = clima_card:getNumberString()
		local card_id = clima_card:getEffectiveId()
		local card_str
		if suit == "spade" then
			card_str = ("lightning:clima[%s:%s]=%d"):format(suit, number, card_id)
		elseif suit == "heart" then
			card_str = ("rain:clima[%s:%s]=%d"):format(suit, number, card_id)
		elseif suit == "diamond" then
			card_str = ("tornado:clima[%s:%s]=%d"):format(suit, number, card_id)
		end
		new_card = sgs.Card_Parse(card_str)
		assert(new_card)
		return new_card
	end
end

--Forecast
sgs.ai_skill_cardask["@forecast-card"] = function(self, data)
	local judge = data:toJudge()
	local cards = sgs.QList2Table(self.player:getHandcards())

	if #cards == 0 then return "." end
	local card_id = self:getRetrialCardId(cards, judge)
	if card_id == -1 then
		if self:needRetrial(judge) then
			self:sortByUseValue(cards, true)
			if self:getUseValue(judge.card) > self:getUseValue(cards[1]) then
				return "@ForecastCard[" .. cards[1]:getSuitString() .. ":" .. cards[1]:getNumberString() .."]=" .. cards[1]:getId()
			end
		end
	elseif self:needRetrial(judge) or self:getUseValue(judge.card) > self:getUseValue(sgs.Bang:getCard(card_id)) then
		local card = sgs.Bang:getCard(card_id)
		return "@ForecastCard[" .. card:getSuitString() .. ":" .. card:getNumberString() .. "]=" .. card_id
	end
	
	return "."
end

--Mirage
sgs.ai_skill_invoke.mirage = function(self, data)
	return true
end

sgs.ai_skill_cardchosen.mirage = function(self, who, flags)
	local judges = who:getJudgingArea()
	return judges[0]
end

--Black Feet
sgs.ai_view_as.blackfeet = function(card, player, card_place)
	if card_place ~= sgs.Player_HandArea then
		return
	end

	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card:inherits("Slash") and card:getSuit() == sgs.Card_Club then
		return ("wine:blackfeet[%s:%s]=%d"):format(suit, number, card_id)
	elseif card:inherits("Jink") then
		return ("fire_slash:blackfeet[%s:%s]=%d"):format(suit, number, card_id)
	end
end

--Lie
lie_skill={}
lie_skill.name="lie"
table.insert(sgs.ai_skills, lie_skill)
lie_skill.getTurnUseCard = function(self)
	local card
	local hcards = self.player:getCards("h")
	hcards = sgs.QList2Table(hcards)
	self:sortByUseValue(hcards, true)

	for _, hcard in ipairs(hcards) do
		if hcard:getSuit() == sgs.Card_Heart then
			card = hcard
			break
		end
	end

	if card then
		card = sgs.Card_Parse("@LieCard=" .. card:getEffectiveId())
		assert(card)
		return card
	end

	return nil
end

sgs.ai_skill_use_func.LieCard=function(card,use,self)
	local target
	self:sort(self.friends, "handcard")
	local friends = self.friends
	for _, friend in ipairs(friends) do
		target = friend
		break
	end

	use.card=card
	
	if use.to then
		if target then
			use.to:append(target)
		else
			use.to:append(self.player)
		end
	end
end

sgs.ai_use_value.LieCard = 11
sgs.ai_use_priority.LieCard = 7

-- Shark on Tooth
sgs.ai_skill_invoke.sharkontooth = function(self, data)
	local room = self.player:getRoom()
	local friend_count = 0
	local enemy_count = 0

	for _,target in sgs.qlist(room:getOtherPlayers(self.player)) do
		if self.player:inMyAttackRange(target) and self:isWeak(target) then
			if self:isFriend(target) then
				friend_count = friend_count + 1
			elseif self:isEnemy(target) then
				enemy_count = enemy_count + 1
			end
		end
	end

	if friend_count <= enemy_count then
		return true
	end

	return false
end

--IronPunch
sgs.ai_view_as.ironpunch = function(card, player, card_place)
	if player:getWeapon() then return end

	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	return ("slash:ironpunch[%s:%s]=%d"):format(suit, number, card_id)
end
