// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomGameViewportClient.h"
#include "GameMapsSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Console.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#endif
#include "Math/UnrealMathUtility.h"





UCustomGameViewportClient::UCustomGameViewportClient(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{}

bool UCustomGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	int32 ControllerId = EventArgs.ControllerId;

	if (TryToggleFullscreenOnInputKey(EventArgs.Key, EventArgs.Event))
	{
		return true;
	}

	if (IgnoreInput())
	{
		return ViewportConsole ? ViewportConsole->InputKey(ControllerId, EventArgs.Key, EventArgs.Event, EventArgs.AmountDepressed, EventArgs.IsGamepad()) : false;
	}

	const int32 NumLocalPlayers = GetWorld() ? GetWorld()->GetGameInstance()->GetNumLocalPlayers() : 0;

	if (NumLocalPlayers > 1 && EventArgs.Key.IsGamepadKey() && GetDefault<UGameMapsSettings>()->bOffsetPlayerGamepadIds && !EventArgs.Key.GetFName().ToString().Contains("MotionController") && !EventArgs.Key.GetFName().ToString().Contains("Oculus"))
	{
		++ControllerId;
	}
	else if (EventArgs.Viewport->IsPlayInEditorViewport() && EventArgs.Key.IsGamepadKey())
	{
		GEngine->RemapGamepadControllerIdForPIE(this, ControllerId);
	}

#if WITH_EDITOR
	// Give debugger commands a chance to process key binding
	if (OnGameViewportInputKey().IsBound())
	{
		if (OnGameViewportInputKey().Execute(EventArgs.Key, FSlateApplication::Get().GetModifierKeys(), EventArgs.Event))
		{
			return true;
		}
	}
#endif

	// route to subsystems that care
	bool bResult = (ViewportConsole ? ViewportConsole->InputKey(ControllerId, EventArgs.Key, EventArgs.Event, EventArgs.AmountDepressed, EventArgs.IsGamepad()) : false);

	if (!bResult)
	{
		ULocalPlayer* const TargetPlayer = GEngine->GetLocalPlayerFromControllerId(this, ControllerId);
		if (TargetPlayer && TargetPlayer->PlayerController)
		{
			bResult = TargetPlayer->PlayerController->InputKey(EventArgs.Key, EventArgs.Event, EventArgs.AmountDepressed, EventArgs.IsGamepad());
		}

		// A gameviewport is always considered to have responded to a mouse buttons to avoid throttling
		if (!bResult && EventArgs.Key.IsMouseButton())
		{
			bResult = true;
		}
	}

#if WITH_EDITOR
	// For PIE, let the next PIE window handle the input if none of our players did
	// (this allows people to use multiple controllers to control each window)
	if (!bResult && ControllerId > NumLocalPlayers - 1 && EventArgs.Viewport->IsPlayInEditorViewport())
	{
		UGameViewportClient* NextViewport = GEngine->GetNextPIEViewport(this);
		if (NextViewport)
		{
			FInputKeyEventArgs NextViewportEventArgs = EventArgs;
			NextViewportEventArgs.ControllerId = ControllerId - NumLocalPlayers;
			bResult = NextViewport->InputKey(NextViewportEventArgs);
		}
	}
#endif

	return bResult;

}

bool UCustomGameViewportClient::InputAxis(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (IgnoreInput())
	{
		return false;
	}

	const int32 NumLocalPlayers = World ? World->GetGameInstance()->GetNumLocalPlayers() : 0;

	if (NumLocalPlayers > 1 && Key.IsGamepadKey() && GetDefault<UGameMapsSettings>()->bOffsetPlayerGamepadIds && !Key.GetFName().ToString().Contains("MotionController") && !Key.GetFName().ToString().Contains("Oculus"))
	{
		++ControllerId;
	}
	else if (InViewport->IsPlayInEditorViewport() && Key.IsGamepadKey())
	{
		GEngine->RemapGamepadControllerIdForPIE(this, ControllerId);
	}

	bool bResult = false;

	// Don't allow mouse/joystick input axes while in PIE and the console has forced the cursor to be visible.  It's
	// just distracting when moving the mouse causes mouse look while you are trying to move the cursor over a button
	// in the editor!
	if (!(InViewport->IsSlateViewport() && InViewport->IsPlayInEditorViewport()) || ViewportConsole == NULL || !ViewportConsole->ConsoleActive())
	{
		// route to subsystems that care
		if (ViewportConsole != NULL)
		{
			bResult = ViewportConsole->InputAxis(ControllerId, Key, Delta, DeltaTime, NumSamples, bGamepad);
		}
		if (!bResult)
		{
			ULocalPlayer* const TargetPlayer = GEngine->GetLocalPlayerFromControllerId(this, ControllerId);
			if (TargetPlayer && TargetPlayer->PlayerController)
			{
				bResult = TargetPlayer->PlayerController->InputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
			}
		}

		// For PIE, let the next PIE window handle the input if none of our players did
		// (this allows people to use multiple controllers to control each window)
		if (!bResult && ControllerId > NumLocalPlayers - 1 && InViewport->IsPlayInEditorViewport())
		{
			UGameViewportClient *NextViewport = GEngine->GetNextPIEViewport(this);
			if (NextViewport)
			{
				bResult = NextViewport->InputAxis(InViewport, ControllerId - NumLocalPlayers, Key, Delta, DeltaTime, NumSamples, bGamepad);
			}
		}

		if (InViewport->IsSlateViewport() && InViewport->IsPlayInEditorViewport())
		{
			// Absorb all keys so game input events are not routed to the Slate editor frame
			bResult = true;
		}
	}

	return bResult;
}