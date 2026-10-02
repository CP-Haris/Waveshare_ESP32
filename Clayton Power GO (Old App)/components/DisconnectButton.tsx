// components/DisconnectButton.tsx
import React from 'react';
import { TouchableOpacity, Text, StyleSheet } from 'react-native';

interface DisconnectButtonProps {
  title: string;
  onPress: () => void;
  disabled?: boolean;
}

const DisconnectButton: React.FC<DisconnectButtonProps> = ({ title, onPress, disabled }) => {
  return (
    <TouchableOpacity
      style={[
        styles.button,
        disabled ? styles.disabledButton : styles.enabledButton,
      ]}
      onPress={!disabled ? onPress : undefined}
      disabled={disabled}
    >
      <Text style={disabled ? styles.disabledText : styles.enabledText}>{title}</Text>
    </TouchableOpacity>
  );
};

const styles = StyleSheet.create({
  button: {
    width: 100,
    height: 40,
    padding: 8,
    borderRadius: 20,
    justifyContent: 'center',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
    elevation: 5,
    marginLeft: 10,
  },
  enabledButton: {
    backgroundColor: '#ffffff',
  },
  disabledButton: {
    backgroundColor: '#cccccc',
  },
  enabledText: {
    color: '#000000',
    fontSize: 12,
    fontWeight: '600',
  },
  disabledText: {
    color: '#ffffff',
    fontSize: 12,
    fontWeight: '600',
  },
});

export default DisconnectButton;
