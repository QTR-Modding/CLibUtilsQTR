#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "FormReader.hpp"
#include "Serialization.hpp"

namespace clib_utilsQTR {
    struct DFSaveData {
        RE::FormID dyn_formid = 0;
        std::pair<bool, uint32_t> custom_id = {false, 0};
        float acteff_elapsed = -1.f;
    };

    using DFSaveDataLHS = std::pair<RE::FormID, std::string>;
    using DFSaveDataRHS = std::vector<DFSaveData>;

    class DFSaveLoadData : public Serialization::BaseData<DFSaveDataLHS, DFSaveDataRHS> {
    public:
        [[nodiscard]] bool Save(SKSE::SerializationInterface* serializationInterface) override;

        [[nodiscard]] bool Save(SKSE::SerializationInterface* serializationInterface, std::uint32_t type,
                                std::uint32_t version) override;

        [[nodiscard]] bool Load(SKSE::SerializationInterface* serializationInterface) override;
    };

    inline bool DFSaveLoadData::Save(SKSE::SerializationInterface* serializationInterface) {
        assert(serializationInterface);
        Locker locker(m_Lock);

        const auto numRecords = m_Data.size();
        if (!serializationInterface->WriteRecordData(numRecords)) {
            SKSE::log::error("Failed to save {} data records", numRecords);
            return false;
        }

        for (const auto& [lhs, rhs] : m_Data) {
            // we serialize formid, editorid, and refid separately
            std::uint32_t formid = lhs.first;
            if (!serializationInterface->WriteRecordData(formid)) {
                SKSE::log::error("Failed to save FormID {:x}", formid);
                return false;
            }

            if (!Serialization::write_string(serializationInterface, lhs.second)) {
                SKSE::log::error("Failed to save EditorID");
                return false;
            }

            // save the number of rhs records
            const auto numRhsRecords = rhs.size();
            if (!serializationInterface->WriteRecordData(numRhsRecords)) {
                SKSE::log::error("Failed to save the size {} of rhs records", numRhsRecords);
                return false;
            }

            for (const auto& rhs_ : rhs) {
                if (!serializationInterface->WriteRecordData(rhs_)) {
                    SKSE::log::error("Failed to save data");
                    return false;
                }
            }
        }
        return true;
    }

    inline bool DFSaveLoadData::Save(SKSE::SerializationInterface* serializationInterface, const std::uint32_t type,
                              const std::uint32_t version) {
        if (!serializationInterface->OpenRecord(type, version)) {
            SKSE::log::error("Failed to open record for Data Serialization!");
            return false;
        }

        return Save(serializationInterface);
    }

    inline bool DFSaveLoadData::Load(SKSE::SerializationInterface* serializationInterface) {
        assert(serializationInterface);

        Locker locker(m_Lock);
        m_Data.clear();

        std::size_t recordDataSize;
        if (serializationInterface->ReadRecordData(recordDataSize) != sizeof(recordDataSize)) return false;
        SKSE::log::trace("Loading data from serialization interface with size: {}", recordDataSize);

        for (std::size_t i = 0; i < recordDataSize; i++) {
            DFSaveDataRHS rhs;

            RE::FormID formid;
            if (serializationInterface->ReadRecordData(formid) != sizeof(formid)) return false;

            std::string editorid;
            if (!Serialization::read_string(serializationInterface, editorid)) {
                SKSE::log::error("Failed to read EditorID");
                return false;
            }

            std::size_t rhsSize;
            if (serializationInterface->ReadRecordData(rhsSize) != sizeof(rhsSize)) return false;

            for (std::size_t j = 0; j < rhsSize; j++) {
                DFSaveData rhs_;
                if (serializationInterface->ReadRecordData(rhs_) != sizeof(rhs_)) return false;
                rhs.push_back(rhs_);
            }

            // Consume the complete entry before skipping an unresolved base.
            if (!serializationInterface->ResolveFormID(formid, formid)) {
                SKSE::log::error("Failed to resolve form ID, 0x{:X}.", formid);
                continue;
            }
            m_Data[{formid, editorid}] = std::move(rhs);
        }

        return true;
    }

    struct ActEff {
        RE::FormID baseFormid;
        RE::FormID dynamicFormid;
        float elapsed;
        std::pair<bool, uint32_t> custom_id;
    };

    /// Owns a per-plugin bank of dynamic forms. Call engine-facing operations on
    /// the game thread; the internal locks do not make arbitrary engine calls safe.
    ///
    /// Resolve a base form, then fetch/create a derivative with a stable caller-defined ID:
    /// @code
    /// auto* tracker = clib_utilsQTR::DynamicFormTracker::GetSingleton();
    /// const auto id = tracker->FetchCreate<RE::AlchemyItem>(base->GetFormID(), "", custom_id);
    /// @endcode
    /// custom_id identifies the derivative within that base form's bank; it is not
    /// a Skyrim FormID. Use std::nullopt when no custom ID is needed.
    ///
    /// Save: SendData(), then Save(serialization, your_record_type, your_version).
    /// Load: Reset() once before reading records; Load(serialization) for this
    /// tracker's record; ReceiveData() after all records have been read.
    /// The caller registers SKSE callbacks and validates record types/versions.
    /// After ReceiveData(), reserve required saved IDs, restore caller-specific
    /// form properties, then call ApplyMissingActiveEffects() for player effects.
    /// Reset retains the session's form bank; it does not delete all dynamic forms.
    /// The serializer preserves AoT's existing field layout.
    class DynamicFormTracker : public DFSaveLoadData {
        template <class T>
        static void copyComponent(RE::TESForm* from, RE::TESForm* to) {
            auto fromT = from->As<T>();

            auto toT = to->As<T>();

            if (fromT && toT) {
                toT->CopyComponent(fromT);
            }
        }

        // created form bank during the session. Create populates this.
        std::map<std::pair<RE::FormID, std::string>, std::unordered_set<RE::FormID>> forms;
        // Session ownership survives Reset; saved IDs alone do not establish ownership.
        std::unordered_map<RE::FormID, const RE::TESForm*> owned_forms; // guarded by forms_mutex
        std::unordered_map<RE::FormID, uint32_t> customIDforms; // Fetch populates this

        std::unordered_set<RE::FormID> active_forms; // _yield populates this
        std::unordered_set<RE::FormID> protected_forms;
        std::unordered_set<RE::FormID> deleted_forms;


        std::shared_mutex forms_mutex;
        std::shared_mutex customIDforms_mutex;
        std::shared_mutex active_forms_mutex;
        std::shared_mutex protected_forms_mutex;
        std::shared_mutex deleted_forms_mutex;
        std::shared_mutex act_effs_mutex;

        bool block_create = false;

        //std::map<RE::FormID,float> act_effs;
        std::vector<ActEff> act_effs; // save file specific

        void CleanseFormsets() {
            for (auto it = forms.begin(); it != forms.end(); ++it) {
                auto& [base, formset] = *it;
                for (auto it2 = formset.begin(); it2 != formset.end();) {
                    const auto base_form = RE::TESForm::LookupByID(base.first);
                    const auto newForm = RE::TESForm::LookupByID(*it2);
                    const auto refForm = RE::TESForm::LookupByID<RE::TESObjectREFR>(*it2);
                    if (!newForm || refForm || (base_form && !CanUseForm(base_form, newForm))) {
                        SKSE::log::trace("Form with ID {:x} does not exist. Removing from formset.", *it2);
                        std::unique_lock lock(forms_mutex);
                        std::unique_lock lock2(customIDforms_mutex);
                        std::unique_lock lock3(active_forms_mutex);
                        customIDforms.erase(*it2);
                        active_forms.erase(*it2);
                        Unreserve(*it2);
                        it2 = formset.erase(it2);
                        //deleted_forms.erase(*it2);
                    } else {
                        ++it2;
                    }
                }
            }
        }

        [[nodiscard]] float GetActiveEffectElapsed(const RE::FormID dyn_formid) {
            std::shared_lock lock(act_effs_mutex);
            for (const auto& act_eff : act_effs) {
                if (act_eff.dynamicFormid == dyn_formid) {
                    return act_eff.elapsed;
                }
            }
            return -1.f;
        }

        [[nodiscard]] bool IsTracked(const RE::FormID dynamic_formid) {
            std::shared_lock lock(forms_mutex);
            for (const auto& dyn_formset : forms | std::views::values) {
                if (dyn_formset.contains(dynamic_formid)) {
                    return true;
                }
            }
            return false;
        }

        void ReviveDynamicForm(RE::TESForm* fake, RE::TESForm* base, const RE::FormID setFormID = 0) {
            fake->Copy(base);
            const auto weaponBaseForm = base->As<RE::TESObjectWEAP>();

            const auto weaponNewForm = fake->As<RE::TESObjectWEAP>();

            const auto bookBaseForm = base->As<RE::TESObjectBOOK>();

            const auto bookNewForm = fake->As<RE::TESObjectBOOK>();

            const auto ammoBaseForm = base->As<RE::TESAmmo>();

            const auto ammoNewForm = fake->As<RE::TESAmmo>();

            if (weaponNewForm && weaponBaseForm) {
                weaponNewForm->firstPersonModelObject = weaponBaseForm->firstPersonModelObject;

                weaponNewForm->weaponData = weaponBaseForm->weaponData;

                weaponNewForm->criticalData = weaponBaseForm->criticalData;

                weaponNewForm->attackSound = weaponBaseForm->attackSound;

                weaponNewForm->attackSound2D = weaponBaseForm->attackSound2D;

                weaponNewForm->attackSound = weaponBaseForm->attackSound;

                weaponNewForm->attackFailSound = weaponBaseForm->attackFailSound;

                weaponNewForm->idleSound = weaponBaseForm->idleSound;

                weaponNewForm->equipSound = weaponBaseForm->equipSound;

                weaponNewForm->unequipSound = weaponBaseForm->unequipSound;

                weaponNewForm->soundLevel = weaponBaseForm->soundLevel;

                weaponNewForm->impactDataSet = weaponBaseForm->impactDataSet;

                weaponNewForm->templateWeapon = weaponBaseForm->templateWeapon;

                weaponNewForm->embeddedNode = weaponBaseForm->embeddedNode;
            } else if (bookBaseForm && bookNewForm) {
                bookNewForm->data.flags = bookBaseForm->data.flags;

                bookNewForm->data.teaches.spell = bookBaseForm->data.teaches.spell;

                bookNewForm->data.teaches.actorValueToAdvance = bookBaseForm->data.teaches.actorValueToAdvance;

                bookNewForm->data.type = bookBaseForm->data.type;

                bookNewForm->inventoryModel = bookBaseForm->inventoryModel;

                bookNewForm->itemCardDescription = bookBaseForm->itemCardDescription;
            } else if (ammoBaseForm && ammoNewForm) {
                ammoNewForm->GetRuntimeData().data.damage = ammoBaseForm->GetRuntimeData().data.damage;

                ammoNewForm->GetRuntimeData().data.flags = ammoBaseForm->GetRuntimeData().data.flags;

                ammoNewForm->GetRuntimeData().data.projectile = ammoBaseForm->GetRuntimeData().data.projectile;
            }
            /*else {
                new_form->Copy(baseForm);
            }*/

            copyComponent<RE::TESDescription>(base, fake);

            copyComponent<RE::BGSKeywordForm>(base, fake);

            copyComponent<RE::BGSPickupPutdownSounds>(base, fake);

            copyComponent<RE::TESModelTextureSwap>(base, fake);

            copyComponent<RE::TESModel>(base, fake);

            copyComponent<RE::BGSMessageIcon>(base, fake);

            copyComponent<RE::TESIcon>(base, fake);

            copyComponent<RE::TESFullName>(base, fake);

            copyComponent<RE::TESValueForm>(base, fake);

            copyComponent<RE::TESWeightForm>(base, fake);

            copyComponent<RE::BGSDestructibleObjectForm>(base, fake);

            copyComponent<RE::TESEnchantableForm>(base, fake);

            copyComponent<RE::BGSBlockBashData>(base, fake);

            copyComponent<RE::BGSEquipType>(base, fake);

            copyComponent<RE::TESAttackDamageForm>(base, fake);

            copyComponent<RE::TESBipedModelForm>(base, fake);

            copyComponent<RE::BGSBipedObjectForm>(base, fake);

            copyComponent<RE::TESRaceForm>(base, fake);

            if (setFormID != 0) fake->SetFormID(setFormID, false);
            std::unique_lock lock(forms_mutex);
            owned_forms[fake->GetFormID()] = fake;
        }

        template <typename T>
        RE::FormID Create(T* baseForm, const RE::FormID setFormID = 0) {
            if (block_create) return 0;

            if (!baseForm) {
                SKSE::log::error("Real form is null for baseForm.");
                return 0;
            }

            const auto base_formid = baseForm->GetFormID();
            const auto base_editorid = clib_util::editorID::get_editorID(baseForm);

            if (base_editorid.empty()) {
                SKSE::log::error("Failed to get editorID for baseForm.");
                return 0;
            }

            auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());

            RE::TESForm* new_form = factory->Create();

            // new_form = baseForm->CreateDuplicateForm(true, (void*)new_form)->As<T>();

            if (!new_form) {
                SKSE::log::error("Failed to create new form.");
                return 0;
            }
            SKSE::log::trace("Original form id: {:x}", new_form->GetFormID());

            if (forms[{base_formid, base_editorid}].contains(setFormID)) {
                SKSE::log::warn("Form with ID {:x} already exist for baseid {} and editorid {}.", setFormID, base_formid,
                             base_editorid);
                ReviveDynamicForm(new_form, baseForm);
            } else ReviveDynamicForm(new_form, baseForm, setFormID);

            const auto new_formid = new_form->GetFormID();

            SKSE::log::trace("Created form with type: {}, Base ID: {:x}, Name: {}",
                          RE::FormTypeToString(new_form->GetFormType()), new_form->GetFormID(), new_form->GetName());

            if (auto lock = std::unique_lock(forms_mutex); !forms[{base_formid, base_editorid}].insert(new_formid).second) {
                lock.unlock();
                SKSE::log::error("Failed to insert new form into forms.");
                if (!_delete({base_formid, base_editorid}, new_formid) && !deleted_forms.contains(new_formid)) {
                    SKSE::log::critical("Failed to delete form with ID {:x}.", new_formid);
                }
                return 0;
            }

            if (new_formid >= 0xFF3DFFFF) {
                SKSE::log::critical("Dynamic FormID limit reached!!!!!!");
                block_create = true;
                if (!_delete({base_formid, base_editorid}, new_formid) && !deleted_forms.contains(new_formid)) {
                    SKSE::log::critical("Failed to delete form with ID {:x}.", new_formid);
                }
                return 0;
            }

            return new_formid;
        }

        RE::FormID GetByCustomID(const uint32_t custom_id, const RE::FormID base_formid, const std::string& base_editorid) {
            const auto formset = GetFormSet(base_formid, base_editorid);
            if (formset.empty()) return 0;
            std::shared_lock lock(customIDforms_mutex);
            RE::FormID result = 0;
            for (const auto _formid : formset) {
                if (!customIDforms.contains(_formid) || customIDforms.at(_formid) != custom_id) continue;
                if (IsActive(_formid)) return _formid;
                if (!result || _formid < result) result = _formid;
            }
            return result;
        }

        // makes it active
        const RE::TESForm* _yield(const RE::FormID dynamic_formid, RE::TESForm* base_form) {
            if (const auto newForm = RE::TESForm::LookupByID(dynamic_formid)) {
                if (!CanUseForm(base_form, newForm)) {
                    SKSE::log::error("Underlying check failed for form with ID {:x}.", dynamic_formid);
                    return nullptr;
                }
                if (std::strlen(newForm->GetName()) == 0) {
                    ReviveDynamicForm(newForm, base_form);
                }
                if (auto lock = std::unique_lock(active_forms_mutex); active_forms.insert(dynamic_formid).second) {
                    lock.unlock();
                    Unreserve(dynamic_formid);
                    if (auto lock2 = std::shared_lock(active_forms_mutex); active_forms.size() > form_limit) {
                        SKSE::log::warn("Active dynamic forms limit reached!!!");
                        block_create = true;
                    }
                }

                return newForm;
            }
            return nullptr;
        }

        bool _delete(const std::pair<RE::FormID, std::string>& base, const RE::FormID dynamic_formid,
                     const bool delete_protected = false) {
            if (auto lock = std::shared_lock(protected_forms_mutex);
                !delete_protected && protected_forms.contains(dynamic_formid)) {
                SKSE::log::warn("Form with ID {:x} is protected.", dynamic_formid);
                return false;
            }
            if (auto lock = std::shared_lock(forms_mutex); !forms.contains(base)) return false;

            const auto base_form = RE::TESForm::LookupByID(base.first);
            const auto newForm = RE::TESForm::LookupByID(dynamic_formid);
            const auto refForm = RE::TESForm::LookupByID<RE::TESObjectREFR>(dynamic_formid);

            if (newForm && !refForm && (OwnsForm(newForm) || (base_form && CanUseForm(base_form, newForm)))) {
                if (const auto bound_temp = newForm->As<RE::TESBoundObject>(); bound_temp) {
                    const auto player = RE::PlayerCharacter::GetSingleton();
                    auto player_inventory = player->GetInventory();
                    if (const auto it = player_inventory.find(bound_temp); it != player_inventory.end()) {
                        player->RemoveItem(bound_temp, it->second.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                    }
                }

                //if (auto* virtualMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton()) {
                //    auto* handlePolicy = virtualMachine->GetObjectHandlePolicy();
                //    auto* bindPolicy = virtualMachine->GetObjectBindPolicy();

                //    if (handlePolicy && bindPolicy) {
                //        auto newHandler = handlePolicy->GetHandleForObject(newForm->GetFormType(), newForm);

                //        if (newHandler != handlePolicy->EmptyHandle()) {
                //            auto* vm_scripts_hashmap = &virtualMachine->attachedScripts;
                //            auto newHandlerScripts_it = vm_scripts_hashmap->find(newHandler);

                //            if (newHandlerScripts_it != vm_scripts_hashmap->end()) {
                //                vm_scripts_hashmap[newHandler].clear();
                //            }
                //        }
                //    }
                //}
                SKSE::log::warn("Deleting form with ID: {:x}", dynamic_formid);
                {
                    std::unique_lock lock(forms_mutex);
                    owned_forms.erase(dynamic_formid);
                }
                delete newForm;
                std::unique_lock lock(deleted_forms_mutex);
                deleted_forms.insert(dynamic_formid);
            }
            else if (newForm && !refForm && !base_form) {
                SKSE::log::warn("Keeping form {:08X}: its base is missing and ownership is unverified.", dynamic_formid);
                return false;
            }
            std::unique_lock lock(forms_mutex);
            std::unique_lock lock2(customIDforms_mutex);
            std::unique_lock lock3(active_forms_mutex);
            std::unique_lock lock4(protected_forms_mutex);
            forms[base].erase(dynamic_formid);
            customIDforms.erase(dynamic_formid);
            active_forms.erase(dynamic_formid);
            protected_forms.erase(dynamic_formid);
            return true;
        }

        [[nodiscard]] bool OwnsForm(const RE::TESForm* form) {
            std::shared_lock lock(forms_mutex);
            const auto it = owned_forms.find(form->GetFormID());
            return it != owned_forms.end() && it->second == form;
        }

        [[nodiscard]] bool CanUseForm(const RE::TESForm* underlying, const RE::TESForm* derivative) {
            if (std::strlen(derivative->GetName()) != 0 && !OwnsForm(derivative)) {
                SKSE::log::trace("Rejecting named form {:08X}: not owned by this DFT session.",
                                 derivative->GetFormID());
                return false;
            }
            if (underlying->GetFormType() != derivative->GetFormType()) {
                SKSE::log::trace("Form types do not match: {} vs {}, ID: {:x} vs {:x}",
                              RE::FormTypeToString(underlying->GetFormType()),
                              RE::FormTypeToString(derivative->GetFormType()),
                              underlying->GetFormID(), derivative->GetFormID());
                SKSE::log::trace("Underlying name: {}, Derivative name: {}", underlying->GetName(), derivative->GetName());
                return false;
            }

            // alchemy
            const auto alch_underlying = underlying->As<RE::AlchemyItem>();
            const auto alch_derivative = derivative->As<RE::AlchemyItem>();
            if ((alch_underlying != nullptr) != (alch_derivative != nullptr)) {
                SKSE::log::trace("Alchemy status does not match.");
                return false;
            }

            if (alch_underlying && alch_derivative) {
                if (alch_underlying->IsPoison() != alch_derivative->IsPoison()) {
                    SKSE::log::trace("Poison status does not match.");
                    return false;
                }

                if (alch_underlying->IsFood() != alch_derivative->IsFood()) {
                    SKSE::log::trace("Food status does not match.");
                    return false;
                }

                if (alch_underlying->IsMedicine() != alch_derivative->IsMedicine()) {
                    SKSE::log::trace("Medicine status does not match.");
                    return false;
                }
            }

            // ingredient
            const auto ingr_underlying = underlying->As<RE::IngredientItem>();
            const auto ingr_derivative = derivative->As<RE::IngredientItem>();
            if ((ingr_underlying != nullptr) != (ingr_derivative != nullptr)) {
                SKSE::log::trace("Ingredient status does not match.");
                return false;
            }

            if (ingr_underlying && ingr_derivative) {
                if (ingr_underlying->IsFood() != ingr_derivative->IsFood()) {
                    SKSE::log::trace("Food status does not match.");
                    return false;
                }

                if (ingr_underlying->IsPoison() != ingr_derivative->IsPoison()) {
                    SKSE::log::trace("Poison status does not match.");
                    return false;
                }

                if (ingr_underlying->IsMedicine() != ingr_derivative->IsMedicine()) {
                    SKSE::log::trace("Medicine status does not match.");
                    return false;
                }
            }

            // POPULATE THIS as you enable other modules

            return true;
        }

        static void RestoreMissingEffects(RE::Actor* player, RE::MagicItem* item, const float elapsed) {
            const auto target = player->AsMagicTarget();
            const auto find_effect = [target, item](const RE::Effect* definition) -> RE::ActiveEffect* {
                if (const auto list = target->GetActiveEffectList()) {
                    for (const auto effect : *list) {
                        if (effect && effect->spell == item && effect->effect == definition &&
                            !effect->flags.any(RE::ActiveEffect::Flag::kDispelled)) return effect;
                    }
                }
                return nullptr;
            };
            for (const auto definition : item->effects) {
                if (!definition || !definition->baseEffect) continue;
                if (const auto existing = find_effect(definition)) {
                    SKSE::log::trace("Keeping effect {:08X} of form {:08X}: elapsed {}s, duration {}s.",
                                     definition->baseEffect->GetFormID(), item->GetFormID(),
                                     existing->elapsedSeconds, existing->duration);
                    continue;
                }
                RE::MagicTarget::AddTargetData data{};
                data.caster = player;
                data.magicItem = item;
                data.effect = definition;
                data.magnitude = definition->GetMagnitude();
                data.power = 1.0f;
                data.castingSource = RE::MagicSystem::CastingSource::kInstant;
                const bool added = target->AddTarget(data);
                if (const auto restored = find_effect(definition)) {
                    restored->elapsedSeconds = restored->duration > elapsed ? elapsed : restored->duration - 1;
                    SKSE::log::trace("Restored effect {:08X} of form {:08X}: elapsed {}s, duration {}s.",
                                     definition->baseEffect->GetFormID(), item->GetFormID(),
                                     restored->elapsedSeconds, restored->duration);
                } else {
                    SKSE::log::trace("Effect {:08X} of form {:08X}: AddTarget returned {}, no persistent effect found.",
                                     definition->baseEffect->GetFormID(), item->GetFormID(), added);
                }
            }
        }

        void DeleteForms(const bool delete_all) {
            std::shared_lock lock(forms_mutex);
            for (auto& [base, formset] : forms) {
                size_t index = 0;
                while (index < formset.size()) {
                    auto it = formset.begin();
                    std::advance(it, index);
                    if (delete_all || (!IsActive(*it) && !IsProtected(*it))) {
                        lock.unlock();
                        if (!_delete(base, *it, delete_all)) {
                            index++;
                        }
                        lock.lock();
                    } else index++;
                }
            }
        }

    public:
        static DynamicFormTracker* GetSingleton() {
            static DynamicFormTracker singleton;
            return &singleton;
        }

        const unsigned int form_limit = 10000;

        const char* GetType() override { return "DynamicFormTracker"; }

        /// Return the base form used to create this derivative, or nullptr if untracked.
        [[nodiscard]] RE::TESForm* GetOGFormOfDynamic(const RE::FormID dynamic_formid) {
            std::shared_lock lock(forms_mutex);
            for (const auto& [base_pair, dyn_formset] : forms) {
                if (dyn_formset.contains(dynamic_formid)) {
                    return FormReader::GetFormByID(base_pair.first, base_pair.second);
                }
            }
            return nullptr;
        }

        bool IsActive(const RE::FormID a_formid) {
            std::shared_lock lock(active_forms_mutex);
            return active_forms.contains(a_formid);
        }

        bool IsProtected(const RE::FormID a_formid) {
            std::shared_lock lock(protected_forms_mutex);
            return protected_forms.contains(a_formid);
        }

        std::unordered_set<RE::FormID> GetFormSet(const RE::FormID base_formid, std::string base_editorid = "") {
            if (base_editorid.empty()) {
                base_editorid = FormReader::GetEditorID(base_formid);
                if (base_editorid.empty()) {
                    return {};
                }
            }
            const std::pair key = {base_formid, base_editorid};
            if (auto lock = std::shared_lock(forms_mutex); forms.contains(key)) return forms.at(key);
            return {};
        }

        /// Delete only inactive, unprotected forms.
        void DeleteInactives() {
            SKSE::log::trace("Deleting inactives.");
            DeleteForms(false);
        }

        /// Explicit teardown: delete tracked forms, including active/protected ones.
        /// The caller must first remove or replace their uses in the game world.
        void DeleteAll() {
            SKSE::log::trace("Deleting all.");
            DeleteForms(true);
            CleanseFormsets();
        }

        std::vector<std::pair<RE::FormID, std::string>> GetSourceForms() {
            std::set<std::pair<RE::FormID, std::string>> source_forms;
            std::shared_lock lock(forms_mutex);
            for (const auto& [base, formset] : forms) {
                if (!formset.empty()) source_forms.insert(base);
            }
            lock.unlock();
            std::shared_lock lock2(act_effs_mutex);
            for (const auto& [base_formid, dynamicFormid, elapsed, custom_id] : act_effs) {
                const auto base_form = FormReader::GetFormByID(base_formid);
                if (!base_form) {
                    SKSE::log::error("Failed to get base form.");
                    continue;
                }
                const auto base_editorid = clib_util::editorID::get_editorID(base_form);
                source_forms.insert({base_formid, base_editorid});
            }
            lock2.unlock();

            auto source_forms_vector = std::vector(source_forms.begin(), source_forms.end());

            return source_forms_vector;
        }

        std::vector<RE::FormID> GetDynamicForms() {
            std::vector<RE::FormID> dynamic_forms;
            std::shared_lock lock(forms_mutex);
            for (const auto& formset : forms | std::views::values) {
                for (const auto formid : formset) {
                    dynamic_forms.push_back(formid);
                }
            }
            return dynamic_forms;
        }

        void EditCustomID(const RE::FormID dynamic_formid, const uint32_t custom_id) {
            std::unique_lock lock(customIDforms_mutex);
            if (customIDforms.contains(dynamic_formid)) customIDforms[dynamic_formid] = custom_id;
            else if (IsTracked(dynamic_formid)) customIDforms.insert({dynamic_formid, custom_id});
        }

        /// With a custom ID, return only its assigned form; return 0 if absent.
        /// With std::nullopt, fetch only an inactive form without a custom ID.
        RE::FormID Fetch(const RE::FormID baseFormID, const std::string& baseEditorID,
                     const std::optional<uint32_t> customID) {
            auto* base_form = FormReader::GetFormByID(baseFormID, baseEditorID);

            if (!base_form) {
                SKSE::log::error("Failed to get base form.");
                return 0;
            }

            if (customID.has_value()) {
                const auto new_formid = GetByCustomID(customID.value(), baseFormID, baseEditorID);
                if (const auto dyn_form = _yield(new_formid, base_form)) return dyn_form->GetFormID();
            } else if (const auto formset = GetFormSet(baseFormID, baseEditorID); !formset.empty()) {
                std::shared_lock lock(customIDforms_mutex);
                for (const auto dyn_formid : formset) {
                    if (IsActive(dyn_formid)) continue;
                    if (customIDforms.contains(dyn_formid)) continue;
                    if (const auto dyn_form = _yield(dyn_formid, base_form)) return dyn_form->GetFormID();
                }
            }

            return 0;
        }

        /// Reuse the requested assignment, or an unassigned inactive form, or create one.
        /// A reused/new form receives customID when supplied; other assignments stay intact.
        template <typename T>
        RE::FormID FetchCreate(const RE::FormID baseFormID, const std::string baseEditorID,
                           const std::optional<uint32_t> customID) {
            // TODO merge with Fetch
            auto* base_form = FormReader::GetFormByID<T>(baseFormID, baseEditorID);

            if (!base_form) {
                SKSE::log::error("Failed to get base form.");
                return 0;
            }

            if (customID.has_value()) {
                const auto new_formid = GetByCustomID(customID.value(), baseFormID, baseEditorID);
                if (const auto dyn_form = _yield(new_formid, base_form)) return dyn_form->GetFormID();
            }

            // before creating new one, try to find one from the bank without custom id
            if (const auto dyn_form = FormReader::GetFormByID<T>(Fetch(baseFormID, baseEditorID, {}))) {
                const auto new_formid = dyn_form->GetFormID();
                if (customID.has_value()) EditCustomID(new_formid, customID.value());
                return new_formid;
            }

            if (const auto dyn_form = _yield(Create<T>(base_form), base_form)) {
                const auto new_formid = dyn_form->GetFormID();
                if (customID.has_value()) EditCustomID(new_formid, customID.value());
                return new_formid;
            }

            return 0;
        }

        [[maybe_unused]] void ReviveAll() {
            for (const auto& [base, formset] : forms) {
                auto* base_form = FormReader::GetFormByID(base.first, base.second);
                if (!base_form) {
                    SKSE::log::error("Failed to get base form.");
                    continue;
                }
                for (const auto _formid : formset) {
                    if (const auto dyn_form = _yield(_formid, base_form)) {
                        SKSE::log::info("Revived form with ID: {:x}", dyn_form->GetFormID());
                    }
                }
            }
        }

        void Reserve(const RE::FormID baseID, const std::string& baseEditorID, const RE::FormID dynamic_formid) {
            if (auto lock = std::shared_lock(protected_forms_mutex); protected_forms.contains(dynamic_formid)) return;
            const auto base_form = FormReader::GetFormByID(
                baseID, baseEditorID);
            if (!base_form) {
                SKSE::log::warn("Base form with ID {:x} not found in forms.", baseID);
                return;
            }
            const auto form = FormReader::GetFormByID(dynamic_formid);
            if (!form) {
                SKSE::log::warn("Form with ID {:x} not found in forms.", dynamic_formid);
                return;
            }
            if (!CanUseForm(base_form, form)) {
                SKSE::log::warn("Underlying check failed for form with ID {:x}.", dynamic_formid);
                return;
            }
            ReviveDynamicForm(form, base_form);
            std::unique_lock lock(protected_forms_mutex);
            std::unique_lock lock2(forms_mutex);
            const std::pair base{base_form->GetFormID(), clib_util::editorID::get_editorID(base_form)};
            for (auto& [previous_base, formset] : forms) {
                if (previous_base != base) formset.erase(dynamic_formid);
            }
            forms[base].insert(dynamic_formid);
            protected_forms.insert(dynamic_formid);
        }

        void Unreserve(const RE::FormID dynamic_formid) {
            std::unique_lock lock(protected_forms_mutex);
            protected_forms.erase(dynamic_formid);
        }

        /// Release an assignment for reuse: clears active, custom-ID, and protected state.
        /// The form stays in its original base form's bank and is not deleted.
        void SetInactive(const RE::FormID dynamic_formid) {
            std::scoped_lock lock(active_forms_mutex, customIDforms_mutex, protected_forms_mutex);
            active_forms.erase(dynamic_formid);
            customIDforms.erase(dynamic_formid);
            protected_forms.erase(dynamic_formid);
        }

        size_t GetNDeleted() {
            std::shared_lock lock(deleted_forms_mutex);
            return deleted_forms.size();
        }

        void SendData() {
            SKSE::log::info("--------Sending data (DFT) ---------");
            Clear();

            {
                std::unique_lock lock(act_effs_mutex);
                act_effs.clear();
            }

            const auto act_eff_list = RE::PlayerCharacter::GetSingleton()->AsMagicTarget()->GetActiveEffectList();

            int n_act_effs = 0;
            std::unordered_set<RE::FormID> act_effs_temp;
            if (act_eff_list) {
                for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
                    if (const auto* act_eff = *it; act_eff && act_eff->spell) {
                        if (const auto act_eff_formid = act_eff->spell->GetFormID(); active_forms.contains(act_eff_formid)) {
                            if (act_effs_temp.contains(act_eff_formid))
                                SKSE::log::warn(
                                    "Active effect already exists in act effs.");
                            else n_act_effs++;
                            const auto base_form = GetOGFormOfDynamic(act_eff_formid);
                            if (!base_form) continue;
                            std::shared_lock lock(customIDforms_mutex);
                            std::unique_lock lock2(act_effs_mutex);
                            const uint32_t customid_temp = customIDforms.contains(act_eff_formid)
                                                               ? customIDforms.at(act_eff_formid)
                                                               : 0;
                            act_effs.push_back({.baseFormid = base_form->GetFormID(),
                                                .dynamicFormid = act_eff_formid,
                                                .elapsed = act_eff->elapsedSeconds,
                                                .custom_id = {false, customid_temp}});
                            act_effs_temp.insert(act_eff_formid);
                        }
                    }
                }
            }

            int n_fakes = 0;
            for (const auto& [base_pair, dyn_formset] : forms) {
                const DFSaveDataLHS lhs({base_pair.first, base_pair.second});
                DFSaveDataRHS rhs;
                for (const auto dyn_formid : dyn_formset) {
                    if (!IsActive(dyn_formid) && !IsProtected(dyn_formid))
                        SKSE::log::trace(
                            "Inactive form {:x} found in forms set.", dyn_formid);
                    std::shared_lock lock(customIDforms_mutex);
                    const auto has_customid = customIDforms.contains(dyn_formid);
                    const uint32_t customid = has_customid ? customIDforms.at(dyn_formid) : 0;
                    const float act_eff_elpsd = GetActiveEffectElapsed(dyn_formid);
                    DFSaveData saveData({.dyn_formid = dyn_formid, .custom_id = {has_customid, customid}, .acteff_elapsed =
                                         act_eff_elpsd});
                    rhs.push_back(saveData);
                    n_fakes++;
                }
                if (!rhs.empty()) SetData(lhs, rhs);
            }

            SKSE::log::info("Number of dynamic forms sent: {}", n_fakes);
            SKSE::log::info("Number of active effects sent: {}", n_act_effs);
            SKSE::log::info("--------Data sent (DFT) ---------");
        }

        void ReceiveData() {
            // std::lock_guard<std::mutex> lock(mutex);
            SKSE::log::info("--------Receiving data (DFT) ---------");

            int n_fakes = 0;
            int n_act_effs = 0;
            for (const auto& [lhs, rhs] : m_Data) {
                auto base_formid = lhs.first;
                const auto& base_editorid = lhs.second;
                const auto temp_form = FormReader::GetFormByID(base_formid, base_editorid);
                if (!temp_form) {
                    SKSE::log::critical("Failed to get base form {:08X} ({}).", base_formid, base_editorid);
                    continue;
                }
                base_formid = temp_form->GetFormID();
                for (const auto& [dyn_formid, custom_id, act_eff_elpsd] : rhs) {
                    const auto [has_customid, customid] = custom_id;
                    if (act_eff_elpsd >= 0.f) {
                        std::unique_lock lock(act_effs_mutex);
                        act_effs.push_back({.baseFormid = base_formid, .dynamicFormid = dyn_formid,
                                            .elapsed = act_eff_elpsd, .custom_id =
                                            {has_customid, customid}});
                        n_act_effs++;
                    }
                    if (const auto dyn_form = RE::TESForm::LookupByID(dyn_formid); !dyn_form) {
                        SKSE::log::trace("Dynamic form {:x} does not exist.", dyn_formid);
                        continue;
                    } else {
                        if ([[maybe_unused]] const auto dyn_form_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(
                            dyn_formid)) {
                            SKSE::log::trace("Dynamic form {:x} is a refr with name {}.", dyn_formid, dyn_form->GetName());
                            continue;
                        }
                        if (!CanUseForm(temp_form, dyn_form)) {
                            // bcs load callback happens after the game loads, there is a chance that the game will assign new
                            // stuff to "previously" our dynamic formid especially for stuff like dynamic food which is not
                            // serialized by the game
                            SKSE::log::trace("Underlying check failed for dynamic form {:x} with name {}.", dyn_formid,
                                          dyn_form->GetName());
                            continue;
                        }
                    }

                    {
                        std::unique_lock lock(forms_mutex);
                        const std::pair base{base_formid, base_editorid};
                        // This save owns the ID; remove associations retained from the previous save.
                        for (auto& [previous_base, formset] : forms) {
                            if (previous_base != base) formset.erase(dyn_formid);
                        }
                        forms[base].insert(dyn_formid);
                    }
                    {
                        std::unique_lock lock(customIDforms_mutex);
                        if (has_customid) customIDforms[dyn_formid] = customid;
                        else customIDforms.erase(dyn_formid);
                    }
                    n_fakes++;
                }
            }

            SKSE::log::info("Number of dynamic forms received: {}", n_fakes);
            SKSE::log::info("Number of active effects received: {}", n_act_effs);
            // need to check if formids and editorids are valid
            #ifndef NDEBUG
            Print();
            #endif  // !NDEBUG

            CleanseFormsets();

            SKSE::log::info("--------Data received (DFT) ---------");
        }

        void Reset() {
            // std::lock_guard<std::mutex> lock(mutex);
            //forms.clear();
            CleanseFormsets();
            std::unique_lock lock(customIDforms_mutex);
            std::unique_lock lock2(active_forms_mutex);
            std::unique_lock lock3(protected_forms_mutex);
            std::unique_lock lock4(act_effs_mutex);
            customIDforms.clear();
            active_forms.clear();
            protected_forms.clear();

            //deleted_forms.clear();

            act_effs.clear();
            block_create = false;
        }

        void Print() {
            std::shared_lock lock(forms_mutex);
            for (const auto& [base, formset] : forms) {
                SKSE::log::trace("---------------------Base formid: {:x}, EditorID: {}---------------------", base.first,
                             base.second);
                for (const auto _formid : formset) {
                    if (const auto form = FormReader::GetFormByID(_formid)) {
                        SKSE::log::trace("Dynamic formid: {:x} with name: {}", _formid, form->GetName());
                    }
                }
            }
        }

        /// Snapshot of the loaded player effects whose derivative forms must be ready before applying them.
        [[nodiscard]] std::vector<ActEff> GetPendingActiveEffects() {
            std::shared_lock lock(act_effs_mutex);
            return act_effs;
        }

        void ApplyMissingActiveEffects() {
            std::unordered_map<RE::FormID, float> new_act_effs; // terrible name
            // I need to change the formids in act_effs if they are not valid to valid ones
            for (std::shared_lock lock(act_effs_mutex);
                 auto& [baseFormid, dynamicFormid, elapsed, customid] : act_effs) {
                if (elapsed < 0.f) {
                    SKSE::log::error("Elapsed time is negative. Removing from act effs.");
                    continue;
                }
                const auto& [has_cstmid, custom_id] = customid;
                const auto base_mg_item = FormReader::GetFormByID(baseFormid);
                if (!base_mg_item) {
                    SKSE::log::error("Failed to get base form.");
                    continue;
                }
                const auto editor_id = clib_util::editorID::get_editorID(base_mg_item);
                const auto dyn_formid = has_cstmid
                    ? GetByCustomID(custom_id, baseFormid, editor_id)
                    : dynamicFormid;
                const auto form = RE::TESForm::LookupByID(dyn_formid);
                if (!form || !OwnsForm(form) || !GetFormSet(baseFormid, editor_id).contains(dyn_formid) ||
                    !CanUseForm(base_mg_item, form)) {
                    SKSE::log::trace("Skipping active-effect restoration for unowned or unrestored form {:08X}.",
                                     dyn_formid);
                    continue;
                }
                new_act_effs[dyn_formid] = elapsed;
            }
            {
                std::unique_lock lock(act_effs_mutex);
                act_effs.clear();
            }
            if (new_act_effs.empty()) return;

            const auto player = RE::PlayerCharacter::GetSingleton();
            for (const auto& [formid, elapsed] : new_act_effs) {
                const auto item = FormReader::GetFormByID<RE::MagicItem>(formid);
                if (!item) continue;
                RestoreMissingEffects(player, item, elapsed);
            }
        }
    };
}
