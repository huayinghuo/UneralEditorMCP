#include "MCPBridgeHelpers.h"
#include "Engine/World.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "UObject/UObjectGlobals.h"

namespace MCPBridgeHelpers
{
	UWorld* GetEditorWorld()
	{
		if (!GEditor) return nullptr;
		return GEditor->GetEditorWorldContext().World();
	}

	AActor* FindActorByNameOrLabel(UWorld* World, const FString& Name)
	{
		if (!World) return nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->GetName() == Name || Actor->GetActorLabel() == Name)
				return Actor;
		}
		return nullptr;
	}

	UActorComponent* FindComponentByNameOrClass(AActor* Actor, const FString& Name)
	{
		if (!Actor) return nullptr;
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Comp : Components)
			if (Comp->GetName() == Name || Comp->GetClass()->GetName() == Name)
				return Comp;
		return nullptr;
	}

	TSharedPtr<FJsonObject> BuildActorJson(AActor* Actor, bool bIncludeTransform)
	{
		TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject());
		if (!Actor) return ActorObj;
		ActorObj->SetStringField(TEXT("name"), Actor->GetName());
		ActorObj->SetStringField(TEXT("label"), Actor->GetActorLabel());
		ActorObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
		ActorObj->SetStringField(TEXT("path"), Actor->GetPathName());
		if (bIncludeTransform)
		{
			USceneComponent* Root = Actor->GetRootComponent();
			if (Root)
			{
				TSharedPtr<FJsonObject> TransformObj = MakeShareable(new FJsonObject());
				FVector Loc = Root->GetComponentLocation();
				TransformObj->SetStringField(TEXT("location"),
					FString::Printf(TEXT("X=%.3f Y=%.3f Z=%.3f"), Loc.X, Loc.Y, Loc.Z));
				FRotator Rot = Root->GetComponentRotation();
				TransformObj->SetStringField(TEXT("rotation"),
					FString::Printf(TEXT("P=%.3f Y=%.3f R=%.3f"), Rot.Pitch, Rot.Yaw, Rot.Roll));
				FVector Scale = Root->GetComponentScale();
				TransformObj->SetStringField(TEXT("scale"),
					FString::Printf(TEXT("X=%.3f Y=%.3f Z=%.3f"), Scale.X, Scale.Y, Scale.Z));
				ActorObj->SetObjectField(TEXT("transform"), TransformObj);
			}
		}
		return ActorObj;
	}

	bool DestroyActorInEditor(AActor* Actor)
	{
		if (!Actor) return false;
		UWorld* World = Actor->GetWorld();
		if (!World) return false;
		Actor->Modify();
		return World->EditorDestroyActor(Actor, true);
	}

	// ====== 反射属性读（OutFound 区分不存在 vs 值为空）======

	FString GetActorPropertyAsString(AActor* Actor, const FString& PropertyName, bool& OutFound)
	{
		OutFound = false;
		if (!Actor) return TEXT("");
		FProperty* Property = Actor->GetClass()->FindPropertyByName(*PropertyName);
		if (!Property) return TEXT("");       // 返回空 + OutFound=false 表示属性不存在
		OutFound = true;                       // 属性存在
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
		if (!ValuePtr) return TEXT("");
		FString ExportedText;
		Property->ExportTextItem_Direct(ExportedText, ValuePtr, nullptr, Actor, PPF_None);
		return ExportedText;                   // 可能为 "" 但 OutFound=true
	}

	FString GetComponentPropertyAsString(UActorComponent* Component, const FString& PropertyName, bool& OutFound)
	{
		OutFound = false;
		if (!Component) return TEXT("");
		FProperty* Property = Component->GetClass()->FindPropertyByName(*PropertyName);
		if (!Property) return TEXT("");
		OutFound = true;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
		if (!ValuePtr) return TEXT("");
		FString ExportedText;
		Property->ExportTextItem_Direct(ExportedText, ValuePtr, nullptr, Component, PPF_None);
		return ExportedText;
	}

	// ====== 反射属性写（Modify 在 ImportText 之前 + 检查返回值）======

	bool SetActorPropertyFromString(AActor* Actor, const FString& PropertyName, const FString& ValueStr)
	{
		if (!Actor) return false;
		FProperty* Property = Actor->GetClass()->FindPropertyByName(*PropertyName);
		if (!Property) return false;
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
		if (!ValuePtr) return false;
		Actor->Modify();  // 先标记修改，再写入
		const TCHAR* Result = Property->ImportText_Direct(*ValueStr, ValuePtr, Actor, PPF_None);
		if (!Result) return false;  // ImportText 返回 nullptr 表示解析失败
		Actor->PostEditChange();
		return true;
	}

	bool SetComponentPropertyFromString(UActorComponent* Component, const FString& PropertyName, const FString& ValueStr)
	{
		if (!Component) return false;
		FProperty* Property = Component->GetClass()->FindPropertyByName(*PropertyName);
		if (!Property) return false;
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
		if (!ValuePtr) return false;
		Component->Modify();  // 先标记修改
		const TCHAR* Result = Property->ImportText_Direct(*ValueStr, ValuePtr, Component, PPF_None);
		if (!Result) return false;
		if (AActor* Owner = Component->GetOwner())
			Owner->PostEditChange();
		return true;
	}

	// ====== Spawn / Parse ======

	AActor* SpawnActorInEditor(UWorld* World, const FString& ClassName, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
	{
		if (!World) return nullptr;
		UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		if (!Class)
		{
			FString WithPrefix = TEXT("A") + ClassName;
			Class = FindFirstObject<UClass>(*WithPrefix, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		}
		if (!Class || !Class->IsChildOf(AActor::StaticClass())) return nullptr;
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(Class, Location, Rotation, SpawnParams);
		if (Actor) Actor->SetActorScale3D(Scale);
		return Actor;
	}

	FVector ParseVectorOptional(TSharedPtr<FJsonObject> Payload, const FString& Key, const FVector& Default)
	{
		const TSharedPtr<FJsonObject>* VecObj = nullptr;
		if (!Payload.IsValid() || !Payload->TryGetObjectField(Key, VecObj) || !(*VecObj).IsValid())
			return Default;
		double X = Default.X, Y = Default.Y, Z = Default.Z;
		(*VecObj)->TryGetNumberField(TEXT("x"), X);
		(*VecObj)->TryGetNumberField(TEXT("y"), Y);
		(*VecObj)->TryGetNumberField(TEXT("z"), Z);
		return FVector(static_cast<double>(X), static_cast<double>(Y), static_cast<double>(Z));
	}

	FRotator ParseRotatorFromPayload(TSharedPtr<FJsonObject> Payload, const FString& Key, const FRotator& Default)
	{
		const TSharedPtr<FJsonObject>* RotObj = nullptr;
		if (!Payload.IsValid() || !Payload->TryGetObjectField(Key, RotObj) || !(*RotObj).IsValid())
			return Default;
		double P = Default.Pitch, Y = Default.Yaw, R = Default.Roll;
		(*RotObj)->TryGetNumberField(TEXT("pitch"), P);
		(*RotObj)->TryGetNumberField(TEXT("yaw"), Y);
		(*RotObj)->TryGetNumberField(TEXT("roll"), R);
		return FRotator(static_cast<double>(P), static_cast<double>(Y), static_cast<double>(R));
	}

	void BuildErrorResponse(const FString& Code, const FString& Message, FString& OutErrorCode, FString& OutErrorMessage)
	{
		OutErrorCode = Code;
		OutErrorMessage = Message;
	}
}
