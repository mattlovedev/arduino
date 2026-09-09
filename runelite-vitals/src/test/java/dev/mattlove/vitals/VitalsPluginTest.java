package dev.mattlove.vitals;

import net.runelite.client.RuneLite;
import net.runelite.client.externalplugins.ExternalPluginManager;

public class VitalsPluginTest
{
	public static void main(String[] args) throws Exception
	{
		ExternalPluginManager.loadBuiltin(VitalsPlugin.class);
		RuneLite.main(args);
	}
}
