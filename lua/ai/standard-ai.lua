
--Rubber Pistol
sgs.ai_skill_invoke.rubberpistol = function(self, data)
	return true
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
	if cards:isEmpty() then return "." end

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
