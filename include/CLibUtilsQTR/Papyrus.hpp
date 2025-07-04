#pragma once

namespace Papyrus {

	using VM = RE::BSScript::Internal::VirtualMachine;
	using ObjectPtr = RE::BSTSmartPointer<RE::BSScript::Object>;

	inline RE::VMHandle GetHandle(const RE::TESForm* a_form)
	{
		const auto vm = VM::GetSingleton();
		const auto policy = vm->GetObjectHandlePolicy();
		return policy->GetHandleForObject(a_form->GetFormType(), a_form);
	}

	inline ObjectPtr GetObjectPtr(const RE::TESForm* a_form, const char* a_class, const bool a_create) {
		const auto vm = VM::GetSingleton();
		const auto handle = GetHandle(a_form);

		ObjectPtr object = nullptr;
		if (const bool found = vm->FindBoundObject(handle, a_class, object); !found && a_create) {
			vm->CreateObject2(a_class, object);
			vm->BindObject(object, handle, false);
		}
		return object;
	}

	template <class... Args>
	bool CallFunction(const std::string_view functionClass, const std::string_view function, Args... a_args)
	{
		const auto skyrimVM = RE::SkyrimVM::GetSingleton();
		if (const auto vm = skyrimVM ? skyrimVM->impl : nullptr) {
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			auto args = RE::MakeFunctionArguments(std::forward<Args>(a_args)...);
			auto res = vm->DispatchStaticCall(std::string(functionClass).c_str(), std::string(function).c_str(), args, callback);
			delete args;
			return res;
		}
		return false;
	}

	inline RE::BSScript::Internal::AttachedScript* GetAttachedScript(const std::string& script_name, RE::TESForm* a_form)
	{
		if (auto* virtualMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton()) {
			auto* handlePolicy = virtualMachine->GetObjectHandlePolicy();
			const auto* bindPolicy = virtualMachine->GetObjectBindPolicy();
			if (handlePolicy && bindPolicy) {
				const auto newHandler = handlePolicy->GetHandleForObject(a_form->GetFormType(), a_form);
				if (newHandler != handlePolicy->EmptyHandle()) {
					auto* vm_scripts_hashmap = &virtualMachine->attachedScripts;
					const auto newHandlerScripts_it = vm_scripts_hashmap->find(newHandler);
					if (newHandlerScripts_it != vm_scripts_hashmap->end()) {
						for (auto& script : newHandlerScripts_it->second) {
							if (const auto oti = script->GetTypeInfo()) {
								if (oti->name.c_str() == script_name) {
									return &script;
								}
							}
						}
					}
				}
			}
		}
		return nullptr;
	}
}