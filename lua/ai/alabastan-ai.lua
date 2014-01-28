--Fire Punch
sgs.ai_view_as.firepunch = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card:getSuit() == sgs.Card_Spade then
		return ("fire_slash:firepunch[%s:%s]=%d"):format(suit, number, card_id)
	end
end

--Anti War
sgs.ai_skill_cardask["antiwar-invoke"] = function(self, data)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if #cards == 0 then return end
	self:sortByUseValue(cards, true)

	local damage = data:toDamage()

	local invoke = false
	if self:isFriend(damage.from) and damage.from:hasSkill(sgs.lose_equip_skill) then
		return "$"..cards[1]:getEffectiveId()
	elseif self:isEnemy(damage.from) then
		return "$"..cards[1]:getEffectiveId()
	end

	return "."
end

--Leechcraft
sgs.ai_view_as.leechcraft = function(card, player, card_place)
	if player:getPhase() ~= sgs.Phase_NotActive then return end

	if card:getSuit() == sgs.Card_Club then
		local suit = card:getSuitString()
		local number = card:getNumberString()
		local card_id = card:getEffectiveId()
		return ("wine:leechcraft[%s:%s]=%d"):format(suit, number, card_id)
	end
end

sgs.chopper_suit_value = {
	club = 6
}

--Winter Sakura
sgs.ai_skill_invoke.wintersakura = function(self, data)
	local dying = data:toDying()
	return self:isFriend(dying.who)
end

--Medical Expertise
sgs.ai_skill_invoke.medicalexpertise = function(self, data)
	local dying = data:toDying()
	return self:isFriend(dying.who)
end

--Fleur
local fleur_skill={}
fleur_skill.name="fleur"
table.insert(sgs.ai_skills,fleur_skill)
fleur_skill.getTurnUseCard=function(self)
	if self.player:hasUsed("FleurCard") then
		return 
	end
	if not self.player:isNude() then
		local card
		local card_id
		if self:isEquip("DiamondArmor") and self.player:isWounded() then
			card = sgs.Card_Parse("@FleurCard=" .. self.player:getArmor():getId())
		elseif self.player:getHandcardNum() > self.player:getHp() then
			local cards = self.player:getHandcards()
			cards=sgs.QList2Table(cards)
			
			for _, acard in ipairs(cards) do
				if (acard:inherits("BasicCard") or acard:inherits("EquipCard") or acard:inherits("AllBlue"))
					and not acard:inherits("Wine") and not acard:inherits("Shit") then 
					card_id = acard:getEffectiveId()
					break
				end
			end
		elseif not self.player:getEquips():isEmpty() then
			local player=self.player
			if player:getWeapon() then card_id=player:getWeapon():getId()
			elseif player:getOffensiveHorse() then card_id=player:getOffensiveHorse():getId()
			elseif player:getDefensiveHorse() then card_id=player:getDefensiveHorse():getId()
			elseif player:getArmor() and player:getHandcardNum()<=1 then card_id=player:getArmor():getId()
			end
		end
		if not card_id then
			cards=sgs.QList2Table(self.player:getHandcards())
			for _, acard in ipairs(cards) do
				if (acard:inherits("BasicCard") or acard:inherits("EquipCard") or acard:inherits("AllBlue"))
					and not acard:inherits("Wine") and not acard:inherits("Shit") then 
					card_id = acard:getEffectiveId()
					break
				end
			end
		end
		if not card_id then
			return nil
		else
			card = sgs.Card_Parse("@FleurCard=" .. card_id)
			return card
		end
	end
	return nil
end

sgs.ai_skill_use_func.FleurCard=function(card,use,self)
	local findFriend_maxSlash=function(self,first)
		self:log("Looking for the friend!")
		local maxSlash = 0
		local friend_maxSlash
		for _, friend in ipairs(self.friends_noself) do
			if (self:getCardsNum("Slash", friend)> maxSlash) and not friend:isKongcheng() then
				maxSlash=self:getCardsNum("Slash", friend)
				friend_maxSlash = friend
			end
		end
		if friend_maxSlash then
			local safe = false
			if (first:hasSkill("ganglie") or first:hasSkill("fankui") or first:hasSkill("enyuan")) then
				if (first:getHp()<=1 and first:getHandcardNum()==0) then safe=true end
			elseif (self:getCardsNum("Slash", friend_maxSlash) >= self:getCardsNum("Slash", first)) then safe=true end
			if safe then return friend_maxSlash end
		else self:log("unfound")
		end
		return nil
	end

	if not self.player:hasUsed("FleurCard") then
		self:sort(self.enemies, "hp")
		local males = {}
		local first, second
		local zhugeliang_kongcheng
		local duel = sgs.Bang:cloneCard("duel", sgs.Card_NoSuit, 0)
		for _, enemy in ipairs(self.enemies) do
			--if zhugeliang_kongcheng and #males==1 and self:damageIsEffective(zhugeliang_kongcheng, sgs.DamageStruct_Normal, males[1]) 
				--then table.insert(males, zhugeliang_kongcheng) end
			if not enemy:isKongcheng() and not enemy:hasSkill("wuyan") then
				if enemy:hasSkill("kongcheng") and enemy:isKongcheng() then	zhugeliang_kongcheng=enemy
				else
					if #males == 0 and self:hasTrickEffective(duel, enemy) then table.insert(males, enemy)
					elseif #males == 1 and self:damageIsEffective(enemy, sgs.DamageStruct_Normal, males[1]) then table.insert(males, enemy) end
				end
				if #males >= 2 then	break end
			end
		end
		if (#males==1) and #self.friends_noself>0 then
			self:log("Only 1")
			first = males[1]
			if zhugeliang_kongcheng and self:damageIsEffective(zhugeliang_kongcheng, sgs.DamageStruct_Normal, males[1]) then
				table.insert(males, zhugeliang_kongcheng)
			else
				local friend_maxSlash = findFriend_maxSlash(self,first)
				if friend_maxSlash and self:damageIsEffective(males[1], sgs.DamageStruct_Normal, enemy) then table.insert(males, friend_maxSlash) end
			end
		end
		if (#males >= 2) then
			first = males[1]
			second = males[2]
			local lord = self.room:getLord()
			if (first:getHp()<=1) then
				if self.player:isLord() or sgs.isRolePredictable() then 
					local friend_maxSlash = findFriend_maxSlash(self,first)
					if friend_maxSlash then second=friend_maxSlash end
				elseif not lord:isKongcheng() and (not lord:hasSkill("wuyan")) then 
					if (self.role=="rebel") and (not first:isLord()) and self:damageIsEffective(lord, sgs.DamageStruct_Normal, first) then
						second = lord
					else
						if ((self.role=="loyalist" or (self.role=="renegade") and not (first:hasSkill("ganglie") and first:hasSkill("enyuan"))))
							and	( self:getCardsNum("Slash", first)<=self:getCardsNum("Slash", second)) then
							second = lord
						end
					end
				end
			end

			if first and second and first:objectName() ~= second:objectName() then
				use.card = card
				if use.to then 
					use.to:append(first)
					use.to:append(second)
				end
			end
		end
	end
end

sgs.ai_use_value.FleurCard = 8.5
sgs.ai_use_priority.FleurCard = 4

fleur_filter = function(player, carduse)
	if carduse.card:inherits("FleurCard") then
		sgs.ai_fleur_effect = true
	end
end

table.insert(sgs.ai_choicemade_filter.cardUsed, fleur_filter)

sgs.ai_card_intention.FleurCard = function(card, from, to)
	if sgs.evaluateRoleTrends(to[1]) == sgs.evaluateRoleTrends(to[2]) then
		sgs.updateIntentions(from, to, 40)
	end
end

sgs.dynamic_value.damage_card.FleurCard = true

sgs.ai_chaofeng.robin = 4
