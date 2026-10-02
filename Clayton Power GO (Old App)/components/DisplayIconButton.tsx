import React, { useEffect, useRef } from 'react';
import { TouchableOpacity, Text, StyleSheet, View, Image, Animated } from 'react-native';

export type OperationalState = 
  | 'OS_DISABLED'     // 0xFFFF0000
  | 'OS_OFF'          // 0x00000
  | 'OS_WAKEUP'       // 0x10000
  | 'OS_READY_TO_START' // 0x20000
  | 'OS_STARTING'     // 0x30000
  | 'OS_STOPPING'     // 0x40000
  | 'OS_ON';          // 0x50000

interface PrimaryButtonProps {
  title: string;
  value?: string;
  superCharge?: boolean;
  active: boolean;
  icon: string;
  operationalState?: OperationalState;
  hasFailure?: boolean;
  failureCount?: number;
  onPress?: () => void;
}

const DisplayIconButton: React.FC<PrimaryButtonProps> = ({ 
  title, 
  icon, 
  value, 
  active = false, 
  superCharge, 
  operationalState,
  hasFailure = false,
  failureCount = 0,
  onPress 
}) => {
  const alphaAnim = useRef(new Animated.Value(1)).current;

  // Animation for transitional states (but not for failure states)
  useEffect(() => {
    if (!operationalState || hasFailure) return;
    
    const isTransitional = ['OS_WAKEUP', 'OS_READY_TO_START', 'OS_STARTING', 'OS_STOPPING'].includes(operationalState);
    
    if (isTransitional) {
      // Start alpha animation (pulsing opacity)
      const alphaAnimation = Animated.loop(
        Animated.sequence([
          Animated.timing(alphaAnim, {
            toValue: 0.3,
            duration: 800,
            useNativeDriver: true,
          }),
          Animated.timing(alphaAnim, {
            toValue: 1,
            duration: 800,
            useNativeDriver: true,
          }),
        ])
      );

      alphaAnimation.start();

      return () => {
        alphaAnimation.stop();
      };
    } else {
      // Reset animation for stable states
      alphaAnim.setValue(1);
    }
  }, [operationalState, alphaAnim, hasFailure]);

  const getButtonStyle = () => {
    // Failure state overrides all other states
    if (hasFailure) {
      return [styles.button, styles.buttonFailure];
    }

    if (!operationalState) {
      return [styles.button, active ? styles.buttonActive : styles.buttonInactive];
    }

    const baseStyle = [styles.button];
    
    switch (operationalState) {
      case 'OS_ON':
        return [...baseStyle, styles.buttonOn];
      case 'OS_OFF':
        return [...baseStyle, styles.buttonOff];
      case 'OS_READY_TO_START':
      case 'OS_WAKEUP':
      case 'OS_STARTING':
      case 'OS_STOPPING':
        return [...baseStyle, styles.buttonOn]; // Use same blue color for all transitional states
      case 'OS_DISABLED':
        return [...baseStyle, styles.buttonDisabled];
      default:
        return [...baseStyle, styles.buttonOff];
    }
  };

  const isTransitional = !hasFailure && operationalState && [
    'OS_WAKEUP',
    'OS_READY_TO_START', 
    'OS_STARTING',
    'OS_STOPPING'
  ].includes(operationalState);

  return (
    <View style={styles.container}>
      <Animated.View 
        style={[
          { opacity: isTransitional ? alphaAnim : 1 }
        ]}
      >
        <TouchableOpacity style={getButtonStyle()} onPress={onPress}>
          {icon === 'alternator' &&
              <Image
                  source={require('../assets/icons/alternator.png')}
                  style={{ height: 24, width: 24 }}
              />
          }
          {icon === 'grid' &&
              <Image
                  source={require('../assets/icons/grid.png')}
                  style={{ height: 24, width: 24 }}
              />
          }
          {icon === 'solar' &&
              <Image
                  source={require('../assets/icons/solar.png')}
                  style={{ height: 24, width: 24 }}
              />
          }
          {superCharge &&
              <View style={styles.superCharge}>
                  <Text style={styles.buttonValueText}>SC</Text>
              </View>
          }
        </TouchableOpacity>
      </Animated.View>
      <Text style={styles.buttonText}>{title}</Text>
      <Text style={styles.buttonValueText}>{value}</Text>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    gap: 5,
    flexDirection: 'column',
    alignItems: 'center',
  },
  button: {
    position: 'relative',
    width: 80,
    height: 80,
    padding: 8,
    borderRadius: 40,
    backgroundColor: '#303030',
    justifyContent: 'center',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: {
      width: 0,
      height: 2,
    },
    shadowOpacity: 0.05,
    shadowRadius: 3.84,
    elevation: 0,
  },
  // Legacy styles for backward compatibility
  buttonActive: {
    backgroundColor: '#0075c1',
  },
  buttonInactive: {
    backgroundColor: '#303030',
  },
  // Operational state styles
  buttonOn: {
    backgroundColor: '#0075c1', // Blue - fully active
  },
  buttonOff: {
    backgroundColor: '#404040', // Dark gray - off but available
    borderWidth: 1,
    borderColor: '#666666',
  },
  // Disabled state
  buttonDisabled: {
    backgroundColor: '#8B0000', // Dark red - disabled
    borderWidth: 1,
    borderColor: '#ff4444',
    opacity: 0.6,
  },
  // Failure state - overrides all others
  buttonFailure: {
    backgroundColor: '#DC143C', // Crimson red - component failure
    borderWidth: 2,
    borderColor: '#FF6B6B',
    shadowColor: '#DC143C',
    shadowOffset: {
      width: 0,
      height: 0,
    },
    shadowOpacity: 0.8,
    shadowRadius: 8,
    elevation: 8,
  },
  superCharge: {
    position: 'absolute',
    top: '50%',
    marginTop: -15,
    right: -15,
    width: 40,
    height: 40,
    padding: 4,
    borderRadius: 20,
    backgroundColor: '#121212',
    justifyContent: 'center',
    alignItems: 'center',
    
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },

  buttonValueText: {
    color: '#ffffff',
    fontSize: 12,
    fontWeight: '600',
  },
});

export default DisplayIconButton;
